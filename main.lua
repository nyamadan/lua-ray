-- SDL3 Ray Tracing in Lua

-- Configuration (Called by C++ before SDL init)
function get_config()
    return {
        width = 800,
        height = 600,
        title = "Lua Ray Tracing (Controlled via Lua)"
    }
end

-- State variables
local sdl_width = 800
local sdl_height = 600
local texture = nil
local device = nil
local scene = nil

-- Initialize Application (Called by C++ after SDL init)
function app_init()
    local config = get_config()
    sdl_width = config.width
    sdl_height = config.height

    print("Initializing Embree from Lua...")
    device = EmbreeDevice.new()
    scene = device:create_scene()

    -- Create the texture from the renderer provided by the host (lightuserdata)
    -- 'renderer' global is set by C++ before calling this function
    if renderer == nil then
        print("Error: Renderer is nil")
        return nil
    end
    
    texture = api_create_texture(renderer, sdl_width, sdl_height)

    -- Create scene: Sphere at (0, 0, 0) with radius 0.5
    scene:add_sphere(0.0, 0.0, 0.0, 0.5)
    scene:commit()
    
    -- Perform initial render
    render_scene()
    
    return texture
end

function render_scene()
    print("Rendering scene...")

    local width = sdl_width
    local height = sdl_height
    local aspectRatio = width / height
    local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707

    -- Lock once, write many pixels, then unlock
    local pixels, pitch = api_lock_texture(texture)
    if pixels ~= nil then
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
                    api_draw_pixel_locked(pixels, pitch, x, y, shade, shade, shade)
                else
                    api_draw_pixel_locked(pixels, pitch, x, y, 128, 128, 128) -- Gray background
                end
            end
        end
        api_unlock_texture(texture)
    end
    print("Lua render finished.")
end

-- GUI Callback (called every frame from C++ loop)
function on_frame()
    if ImGui.Begin("Lua Ray Tracer Control") then
        ImGui.Text("Welcome to Lua Ray Tracer!")
        ImGui.Text(string.format("Resolution: %d x %d", sdl_width, sdl_height))
        
        if ImGui.Button("Reload Scene (Not Implemented)") then
            print("Reload requested")
            -- render_scene() -- Re-render on request if needed
        end
    end
    ImGui.End()
end

