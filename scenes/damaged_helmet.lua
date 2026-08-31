local CameraUtils = require("lib.CameraUtils")
local PBR = require("lib.PBR")

local M = {}
local GLTF_NAME = "damaged_helmet"
local GLTF_PATH = "assets/DamagedHelmet.glb"
local TEXTURE_NAMES = {"base", "metallic_roughness", "emissive", "occlusion", "normal"}

local scene, camera, width, height
local vertices, normals, texcoords, indices
local textures = {}

local lights = {
    {position = {2.5, 3.0, 3.5}, color = {38.0, 34.0, 30.0}},
    {position = {-3.0, 1.0, 2.0}, color = {12.0, 16.0, 22.0}}
}

local function rotate_x_90(values)
    for i = 1, #values, 3 do
        local y, z = values[i + 1], values[i + 2]
        values[i + 1], values[i + 2] = -z, y
    end
    return values
end

local function normalize(x, y, z)
    local length = math.sqrt(x * x + y * y + z * z)
    if length < 1e-8 then return 0.0, 0.0, 1.0 end
    return x / length, y / length, z / length
end

local function interpolate(values, components, prim_id, u, v)
    local base = prim_id * 3
    local ids = {indices[base + 1], indices[base + 2], indices[base + 3]}
    local weights = {1.0 - u - v, u, v}
    local result = {}
    for component = 1, components do
        local value = 0.0
        for corner = 1, 3 do
            value = value + values[ids[corner] * components + component] * weights[corner]
        end
        result[component] = value
    end
    return result, ids
end

local function sampled(texture, u, v, srgb)
    local r, g, b = texture:sample(u, v)
    r, g, b = r / 255.0, g / 255.0, b / 255.0
    if srgb then
        return PBR.srgb_to_linear(r), PBR.srgb_to_linear(g), PBR.srgb_to_linear(b)
    end
    return r, g, b
end

local function mapped_normal(prim_id, bary_u, bary_v, uv_u, uv_v)
    local smooth, ids = interpolate(normals, 3, prim_id, bary_u, bary_v)
    local nx, ny, nz = normalize(smooth[1], smooth[2], smooth[3])
    local i0, i1, i2 = ids[1], ids[2], ids[3]
    local function position(index)
        return vertices[index * 3 + 1], vertices[index * 3 + 2], vertices[index * 3 + 3]
    end
    local function uv(index)
        return texcoords[index * 2 + 1], texcoords[index * 2 + 2]
    end
    local x0, y0, z0 = position(i0)
    local x1, y1, z1 = position(i1)
    local x2, y2, z2 = position(i2)
    local u0, v0 = uv(i0)
    local u1, v1 = uv(i1)
    local u2, v2 = uv(i2)
    local e1x, e1y, e1z = x1 - x0, y1 - y0, z1 - z0
    local e2x, e2y, e2z = x2 - x0, y2 - y0, z2 - z0
    local du1, dv1, du2, dv2 = u1 - u0, v1 - v0, u2 - u0, v2 - v0
    local determinant = du1 * dv2 - dv1 * du2
    if math.abs(determinant) < 1e-8 then return nx, ny, nz end
    local reciprocal = 1.0 / determinant
    local tx = (e1x * dv2 - e2x * dv1) * reciprocal
    local ty = (e1y * dv2 - e2y * dv1) * reciprocal
    local tz = (e1z * dv2 - e2z * dv1) * reciprocal
    local projection = tx * nx + ty * ny + tz * nz
    tx, ty, tz = normalize(tx - nx * projection, ty - ny * projection, tz - nz * projection)
    local bx, by, bz = normalize(ny * tz - nz * ty, nz * tx - nx * tz, nx * ty - ny * tx)
    if determinant < 0 then bx, by, bz = -bx, -by, -bz end
    local mx, my, mz = sampled(textures.normal, uv_u, uv_v, false)
    mx, my, mz = mx * 2.0 - 1.0, my * 2.0 - 1.0, mz * 2.0 - 1.0
    return normalize(tx * mx + bx * my + nx * mz,
                     ty * mx + by * my + ny * mz,
                     tz * mx + bz * my + nz * mz)
end

function M.setup(embree_scene, app_data)
    assert(app_data:load_gltf(GLTF_NAME, GLTF_PATH), "Failed to load " .. GLTF_PATH)
    for index, name in ipairs(TEXTURE_NAMES) do
        assert(app_data:load_texture_image(GLTF_NAME .. "_" .. name, GLTF_NAME, index - 1),
               "Missing DamagedHelmet texture " .. tostring(index - 1))
    end
    local source_vertices = assert(app_data:get_gltf_vertices(GLTF_NAME, 0, 0), "Missing positions")
    local mesh_indices = assert(app_data:get_gltf_indices(GLTF_NAME, 0, 0), "Missing indices")
    local geom_id = embree_scene:add_mesh(rotate_x_90(source_vertices), mesh_indices)
    assert(geom_id ~= 4294967295, "Failed to add DamagedHelmet mesh")
end

function M.start(embree_scene, app_data)
    scene, width, height = embree_scene, app_data:width(), app_data:height()
    vertices = rotate_x_90(assert(app_data:get_gltf_vertices(GLTF_NAME, 0, 0)))
    normals = rotate_x_90(assert(app_data:get_gltf_normals(GLTF_NAME, 0, 0)))
    texcoords = assert(app_data:get_gltf_tex_coords(GLTF_NAME, 0, 0))
    indices = assert(app_data:get_gltf_indices(GLTF_NAME, 0, 0))
    textures = {}
    for _, name in ipairs(TEXTURE_NAMES) do
        textures[name] = assert(app_data:get_cached_texture(GLTF_NAME .. "_" .. name))
    end
    camera = CameraUtils.setup_or_sync_camera(camera, app_data, {
        position = {1.15, 0.55, 3.0}, look_at = {0.0, -0.10, -0.20},
        up = {0, 1, 0}, aspect_ratio = width / height, fov = 38.0
    })
end

function M.shade(data, x, y)
    local screen_u = (2.0 * x - width) / width
    local screen_v = (2.0 * y - height) / height
    local ox, oy, oz, dx, dy, dz = camera:generate_ray(screen_u, screen_v)
    local hit, distance, gx, gy, gz, _, prim_id, bary_u, bary_v = scene:intersect(ox, oy, oz, dx, dy, dz)
    local output_y = height - 1 - y
    if not hit then
        local t = math.max(0.0, math.min(1.0, (screen_v + 1.0) * 0.5))
        data:set_pixel(x, output_y, math.floor(18 + 20 * t), math.floor(22 + 22 * t), math.floor(30 + 28 * t))
        return
    end

    local uv = interpolate(texcoords, 2, prim_id, bary_u, bary_v)
    local base_r, base_g, base_b = sampled(textures.base, uv[1], uv[2], true)
    local _, roughness, metallic = sampled(textures.metallic_roughness, uv[1], uv[2], false)
    local ao = select(1, sampled(textures.occlusion, uv[1], uv[2], false))
    local emit_r, emit_g, emit_b = sampled(textures.emissive, uv[1], uv[2], true)
    local nx, ny, nz = mapped_normal(prim_id, bary_u, bary_v, uv[1], uv[2])
    if nx * dx + ny * dy + nz * dz > 0 then nx, ny, nz = -nx, -ny, -nz end
    if gx * dx + gy * dy + gz * dz > 0 then gx, gy, gz = -gx, -gy, -gz end
    local px, py, pz = ox + dx * distance, oy + dy * distance, oz + dz * distance
    local vx, vy, vz = normalize(ox - px, oy - py, oz - pz)
    local color = {base_r * 0.035 * ao + emit_r, base_g * 0.035 * ao + emit_g, base_b * 0.035 * ao + emit_b}

    for _, light in ipairs(lights) do
        local lx, ly, lz = light.position[1] - px, light.position[2] - py, light.position[3] - pz
        local light_distance2 = lx * lx + ly * ly + lz * lz
        local light_distance = math.sqrt(light_distance2)
        lx, ly, lz = lx / light_distance, ly / light_distance, lz / light_distance
        local shadow_hit, shadow_distance = scene:intersect(px + gx * 0.001, py + gy * 0.001, pz + gz * 0.001, lx, ly, lz)
        if not shadow_hit or shadow_distance >= light_distance - 0.002 then
            local radiance = {light.color[1] / light_distance2, light.color[2] / light_distance2, light.color[3] / light_distance2}
            local r, g, b = PBR.evaluate_direct({base_r, base_g, base_b}, metallic, roughness,
                {nx, ny, nz}, {vx, vy, vz}, {lx, ly, lz}, radiance)
            color[1], color[2], color[3] = color[1] + r, color[2] + g, color[3] + b
        end
    end
    data:set_pixel(x, output_y, PBR.to_display_byte(color[1]), PBR.to_display_byte(color[2]), PBR.to_display_byte(color[3]))
end

function M.get_camera() return camera end

function M.cleanup()
    scene, camera, vertices, normals, texcoords, indices = nil, nil, nil, nil, nil, nil
    textures = {}
end

return M
