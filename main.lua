-- SDL3 Ray Tracing in Lua

-- Configuration (Called by C++ before SDL init)
-- Constants
local WIDTH = 800
local HEIGHT = 600

-- Explicit Initialization Sequence
if not app.init_video() then
    print("Error: Failed to init video")
    return
end

local window = app.create_window(WIDTH, HEIGHT, "Lua Ray Tracing (Explicit DI)")
if window == nil then
    print("Error: Failed to create window")
    return
end

local renderer = app.create_renderer(window)
if renderer == nil then
    print("Error: Failed to create renderer")
    return
end

local texture = app.create_texture(renderer, WIDTH, HEIGHT)
if texture == nil then
    print("Error: Failed to create texture")
    return
end

app.configure({
    width = WIDTH,
    height = HEIGHT,
    title = "Lua Ray Tracing (Explicit DI)",
    window = window,
    renderer = renderer,
    texture = texture
})

-- State variables
local sdl_width = WIDTH
local sdl_height = HEIGHT
local device = nil
local scene = nil

function render_scene()
    print("Rendering scene...")

    local width = sdl_width
    local height = sdl_height
    local aspectRatio = width / height
    local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707

    -- Lock once, write many pixels, then unlock
    local pixels, pitch = app.lock_texture(texture)
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
                    app.draw_pixel_locked(pixels, pitch, x, y, shade, shade, shade)
                else
                    app.draw_pixel_locked(pixels, pitch, x, y, 128, 128, 128) -- Gray background
                end
            end
        end
        app.unlock_texture(texture)
    end
    print("Lua render finished.")
end

-- Initialize Application
-- Config is set implicitly by create_window logic, explicit variables maintained here.
sdl_width = WIDTH 
sdl_height = HEIGHT

print("Initializing Embree from Lua...")
device = EmbreeDevice.new()
scene = device:create_scene()

-- Create scene: Sphere at (0, 0, 0) with radius 0.5
scene:add_sphere(0.0, 0.0, 0.0, 0.5)
scene:commit()

-- Perform initial render
render_scene()

-- GUI Callback (called every frame from C++ loop)
function app.on_frame()
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

