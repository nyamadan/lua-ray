-- SDL3 Ray Tracing in Lua - Triangles

local width = 800
local height = 600

function render()
    print("Initializing Embree from Lua...")
    local device = EmbreeDevice.new()
    local scene = device:create_scene()

    -- Create scene: A single triangle
    -- Vertices: (-0.5, -0.5, 0), (0.5, -0.5, 0), (0.0, 0.5, 0)
    -- Camera is at (0,0,1) looking at -z. So z=0 is in front of camera.
    print("Adding triangle...")
    scene:add_triangle(
        -0.5, -0.5, 0.0,
         0.5, -0.5, 0.0,
         0.0,  0.5, 0.0
    )
    
    scene:commit()

    print("Rendering scene...")

    local aspectRatio = width / height
    local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707
    -- Re-normalize light direction
    local lightLen = math.sqrt(lightDirX*lightDirX + lightDirY*lightDirY + lightDirZ*lightDirZ)
    lightDirX, lightDirY, lightDirZ = lightDirX/lightLen, lightDirY/lightLen, lightDirZ/lightLen

    for y = 0, height - 1 do
        for x = 0, width - 1 do
            -- Normalize pixel coordinates [-1, 1]
            local u = (2.0 * x - width) / width
            local v = (2.0 * y - height) / height
            
            -- Flip v because y=0 is top in SDL, but usually +y is up in 3D (or vice versa depending on camera)
            -- in main.lua: v = (2.0 * y - height) / height. if y=0, v=-1. if y=h, v=1.
            -- Camera setup: ox,oy,oz = 0,0,1. dx,dy,dz = u,v,-1.
            -- if v=-1 (top of screen), direction is (.., -1, -1). Pointing down-ish.
            -- if v=1 (bottom), direction is (.., 1, -1). Pointing up-ish.
            -- If we want standard coords (y up), we might want to invert v.
            -- But let's stick to main.lua convention.
            
            u = u * aspectRatio
            
            -- Ray setup (Camera at 0, 0, 1 looking at -z)
            local ox, oy, oz = 0.0, 0.0, 1.0
            local dx, dy, dz = u, v, -1.0

            -- Normalize direction
            local len = math.sqrt(dx*dx + dy*dy + dz*dz)
            dx, dy, dz = dx/len, dy/len, dz/len

            -- Intersect
            local hit, t, nx, ny, nz = scene:intersect(ox, oy, oz, dx, dy, dz)

            if hit then
                -- Diffuse shading
                -- Simple dot product
                local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
                -- Take absolute value for double sided lighting or clamp for single sided
                if diffuse < 0 then diffuse = 0 end 
                -- or just abs(diffuse) if we want to see backfaces too as lit
                -- diffuse = math.abs(diffuse) 
                
                -- Add some ambient
                diffuse = 0.2 + 0.8 * diffuse
                if diffuse > 1.0 then diffuse = 1.0 end

                local shade = math.floor(255 * diffuse)
                
                -- Color the triangle based on normals or just white/flat
                -- Let's make it Red-ish if we want, or just white.
                -- api_draw_pixel(x, y, shade, shade, shade) -- grayscale
                
                -- Let's do a simple color:
                api_draw_pixel(x, y, math.floor(shade * 1.0), math.floor(shade * 0.8), math.floor(shade * 0.8))
            else
                api_draw_pixel(x, y, 50, 50, 60) -- Dark blue-ish background
            end
        end
    end

    print("Lua render finished.")
end

render()
