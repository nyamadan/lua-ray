-- SDL3 Ray Tracing in Lua

local width = 800
local height = 600

function render()
    print("Initializing Embree from Lua...")
    local device = EmbreeDevice.new()
    local scene = device:create_scene()

    -- Create scene: Sphere at (0, 0, 0) with radius 0.5
    scene:add_sphere(0.0, 0.0, 0.0, 0.5)
    scene:commit()

    print("Rendering scene...")

    local aspectRatio = width / height
    local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707

    for y = 0, height - 1 do
        for x = 0, width - 1 do
            -- Normalize pixel coordinates [-1, 1]
            local u = (2.0 * x - width) / width
            local v = (2.0 * y - height) / height
            
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
                local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
                if diffuse < 0 then diffuse = 0 end
                
                local shade = math.floor(255 * diffuse)
                api_draw_pixel(x, y, shade, shade, shade)
            else
                api_draw_pixel(x, y, 128, 128, 128) -- Gray background
            end
        end
    end

    print("Lua render finished.")
    -- Cleanup is handled automatically by Lua garbage collector and C++ destructors
end

-- Call the render function
render()
