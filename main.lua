-- SDL3 Ray Tracing in Lua

local width = 800
local height = 600

function render()
    print("Initializing Embree from Lua...")
    local device = api_embree_new_device()
    local scene = api_embree_new_scene(device)

    -- Create scene: Sphere at (0, 0, 0) with radius 0.5
    api_embree_add_sphere(device, scene, 0.0, 0.0, 0.0, 0.5)
    api_embree_commit_scene(scene)

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
            local hit, t, nx, ny, nz = api_embree_intersect(scene, ox, oy, oz, dx, dy, dz)

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
    
    -- Cleanup
    api_embree_release_scene(scene)
    api_embree_release_device(device)
end

-- Call the render function
render()
