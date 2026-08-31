local CameraUtils = require("lib.CameraUtils")
local PathTracer = require("lib.PathTracer")
local PBR = require("lib.PBR")
local RNG = require("lib.RNG")
local Ray = require("lib.Ray")
local Vec3 = require("lib.Vec3")
local json = require("lib.json")

local M = {}
local GLTF_NAME = "damaged_helmet"
local GLTF_PATH = "assets/DamagedHelmet.glb"
local IDS_KEY = "damaged_helmet_pt_geometry_ids"
local TEXTURE_NAMES = {"base", "metallic_roughness", "emissive", "occlusion", "normal"}
local MAX_SAMPLES, MAX_DEPTH, RR_DEPTH = 128, 8, 4
local LIGHT = {width=2.5, depth=2.0, y=2.8, center_z=-0.2, area=5.0, emission={16,15,14}}

local scene, camera, width, height, pass_index
local vertices, normals, texcoords, indices, geometry_ids
local textures = {}

local function rotate_x_90(values)
    for i=1,#values,3 do
        local y,z=values[i+1],values[i+2]
        values[i+1],values[i+2]=-z,y
    end
    return values
end

local function normalize(v)
    local length=math.sqrt(math.max(v[1]*v[1]+v[2]*v[2]+v[3]*v[3],1e-12))
    return {v[1]/length,v[2]/length,v[3]/length}
end

local function interpolate(values,components,prim_id,u,v)
    local base=prim_id*3
    local ids={indices[base+1],indices[base+2],indices[base+3]}
    local weights={1-u-v,u,v}
    local result={}
    for component=1,components do
        result[component]=0
        for corner=1,3 do
            result[component]=result[component]+values[ids[corner]*components+component]*weights[corner]
        end
    end
    return result,ids
end

local function sampled(texture,u,v,srgb)
    local r,g,b=texture:sample(u,v)
    r,g,b=r/255,g/255,b/255
    if srgb then return PBR.srgb_to_linear(r),PBR.srgb_to_linear(g),PBR.srgb_to_linear(b) end
    return r,g,b
end

local function mapped_normal(prim_id,bary_u,bary_v,uv_u,uv_v)
    local smooth,ids=interpolate(normals,3,prim_id,bary_u,bary_v)
    local n=normalize(smooth)
    local function position(index) return {vertices[index*3+1],vertices[index*3+2],vertices[index*3+3]} end
    local function uv(index) return texcoords[index*2+1],texcoords[index*2+2] end
    local p0,p1,p2=position(ids[1]),position(ids[2]),position(ids[3])
    local u0,v0=uv(ids[1]); local u1,v1=uv(ids[2]); local u2,v2=uv(ids[3])
    local e1={p1[1]-p0[1],p1[2]-p0[2],p1[3]-p0[3]}
    local e2={p2[1]-p0[1],p2[2]-p0[2],p2[3]-p0[3]}
    local du1,dv1,du2,dv2=u1-u0,v1-v0,u2-u0,v2-v0
    local determinant=du1*dv2-dv1*du2
    if math.abs(determinant)<1e-8 then return n end
    local reciprocal=1/determinant
    local tangent=normalize({(e1[1]*dv2-e2[1]*dv1)*reciprocal,
        (e1[2]*dv2-e2[2]*dv1)*reciprocal,(e1[3]*dv2-e2[3]*dv1)*reciprocal})
    local projection=tangent[1]*n[1]+tangent[2]*n[2]+tangent[3]*n[3]
    tangent=normalize({tangent[1]-n[1]*projection,tangent[2]-n[2]*projection,tangent[3]-n[3]*projection})
    local bitangent=normalize({n[2]*tangent[3]-n[3]*tangent[2],
        n[3]*tangent[1]-n[1]*tangent[3],n[1]*tangent[2]-n[2]*tangent[1]})
    if determinant<0 then bitangent={-bitangent[1],-bitangent[2],-bitangent[3]} end
    local x,y,z=sampled(textures.normal,uv_u,uv_v,false)
    x,y,z=x*2-1,y*2-1,z*2-1
    return normalize({tangent[1]*x+bitangent[1]*y+n[1]*z,
        tangent[2]*x+bitangent[2]*y+n[2]*z,tangent[3]*x+bitangent[3]*y+n[3]*z})
end

function M.setup(embree_scene,app_data)
    assert(app_data:load_gltf(GLTF_NAME,GLTF_PATH))
    for index,name in ipairs(TEXTURE_NAMES) do
        assert(app_data:load_texture_image(GLTF_NAME.."_"..name,GLTF_NAME,index-1))
    end
    local positions=rotate_x_90(assert(app_data:get_gltf_vertices(GLTF_NAME,0,0)))
    local mesh_indices=assert(app_data:get_gltf_indices(GLTF_NAME,0,0))
    local ids={helmet=embree_scene:add_mesh(positions,mesh_indices)}
    ids.floor1=embree_scene:add_triangle(-4,-1.2,-4,4,-1.2,-4,4,-1.2,4)
    ids.floor2=embree_scene:add_triangle(-4,-1.2,-4,4,-1.2,4,-4,-1.2,4)
    local x=LIGHT.width/2; local z0=LIGHT.center_z-LIGHT.depth/2; local z1=LIGHT.center_z+LIGHT.depth/2
    ids.light1=embree_scene:add_triangle(-x,LIGHT.y,z0,x,LIGHT.y,z0,x,LIGHT.y,z1)
    ids.light2=embree_scene:add_triangle(-x,LIGHT.y,z0,x,LIGHT.y,z1,-x,LIGHT.y,z1)
    app_data:set_string(IDS_KEY,json.encode(ids))
end

function M.start(embree_scene,app_data)
    scene,width,height=embree_scene,app_data:width(),app_data:height()
    pass_index=tonumber(app_data:get_string("progressive_pass")) or 1
    geometry_ids=json.decode(assert(app_data:get_string(IDS_KEY)))
    vertices=rotate_x_90(assert(app_data:get_gltf_vertices(GLTF_NAME,0,0)))
    normals=rotate_x_90(assert(app_data:get_gltf_normals(GLTF_NAME,0,0)))
    texcoords=assert(app_data:get_gltf_tex_coords(GLTF_NAME,0,0))
    indices=assert(app_data:get_gltf_indices(GLTF_NAME,0,0))
    textures={}
    for _,name in ipairs(TEXTURE_NAMES) do textures[name]=assert(app_data:get_cached_texture(GLTF_NAME.."_"..name)) end
    camera=CameraUtils.setup_or_sync_camera(camera,app_data,{
        position={1.15,0.55,3.0},look_at={0,-0.10,-0.20},up={0,1,0},aspect_ratio=width/height,fov=38
    })
end

local function surface(hit)
    if hit.geom_id==geometry_ids.light1 or hit.geom_id==geometry_ids.light2 then
        return {normal={0,-1,0},emission=LIGHT.emission}
    end
    if hit.geom_id==geometry_ids.floor1 or hit.geom_id==geometry_ids.floor2 then
        return {normal=hit.geometric_normal,emission={0,0,0},
            material={base_color={0.32,0.34,0.38},metallic=0,roughness=0.9}}
    end
    local uv=interpolate(texcoords,2,hit.prim_id,hit.bary_u,hit.bary_v)
    local r,g,b=sampled(textures.base,uv[1],uv[2],true)
    local _,roughness,metallic=sampled(textures.metallic_roughness,uv[1],uv[2],false)
    local er,eg,eb=sampled(textures.emissive,uv[1],uv[2],true)
    return {normal=mapped_normal(hit.prim_id,hit.bary_u,hit.bary_v,uv[1],uv[2]),
        emission={er,eg,eb},material={base_color={r,g,b},metallic=metallic,roughness=roughness}}
end

local function sample_light(point,rng)
    local target={(rng:next()-0.5)*LIGHT.width,LIGHT.y,LIGHT.center_z+(rng:next()-0.5)*LIGHT.depth}
    local delta={target[1]-point[1],target[2]-point[2],target[3]-point[3]}
    local distance2=delta[1]*delta[1]+delta[2]*delta[2]+delta[3]*delta[3]
    local distance=math.sqrt(distance2); local direction={delta[1]/distance,delta[2]/distance,delta[3]/distance}
    local cosine=math.max(direction[2],0)
    if cosine<=0 then return nil end
    return {direction=direction,distance=distance,radiance=LIGHT.emission,pdf=distance2/(cosine*LIGHT.area)}
end

local function light_pdf(from,hit)
    if hit.geom_id~=geometry_ids.light1 and hit.geom_id~=geometry_ids.light2 then return 0 end
    local delta={hit.point[1]-from[1],hit.point[2]-from[2],hit.point[3]-from[3]}
    local distance2=delta[1]*delta[1]+delta[2]*delta[2]+delta[3]*delta[3]
    local cosine=math.max(delta[2]/math.sqrt(distance2),0)
    if cosine<=0 then return 0 end
    return distance2/(cosine*LIGHT.area)
end

function M.shade(data,x,y)
    local rng=RNG.for_pixel(x,y,pass_index)
    local u=(2*(x+rng:next())-width)/width
    local v=(2*(y+rng:next())-height)/height
    local ox,oy,oz,dx,dy,dz=camera:generate_ray(u,v)
    local color=PathTracer.trace_mis(Ray.new(Vec3.new(ox,oy,oz),Vec3.new(dx,dy,dz)),scene,{
        surface=surface,sample_light=sample_light,light_pdf=light_pdf,
        background=function() return {0.004,0.006,0.01} end
    },rng,{max_depth=MAX_DEPTH,rr_depth=RR_DEPTH})
    local output_y=height-1-y
    local r,g,b=data:accumulate_sample(x,output_y,color[1],color[2],color[3])
    data:set_pixel(x,output_y,PBR.to_display_byte(r),PBR.to_display_byte(g),PBR.to_display_byte(b))
end

function M.is_progressive() return true end
function M.get_max_samples() return MAX_SAMPLES end
function M.get_camera() return camera end
function M.cleanup()
    scene,camera,vertices,normals,texcoords,indices,geometry_ids=nil,nil,nil,nil,nil,nil,nil
    textures={}
end

return M
