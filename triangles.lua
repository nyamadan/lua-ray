-- SDL3 Ray Tracing in Lua - Triangles

local WIDTH = 800
local HEIGHT = 600
-- Explicit Initialization Sequence
if not app.init_video() then error("Failed to init video") end
local window = app.create_window(WIDTH, HEIGHT, "Lua Ray Tracing - Triangles")
if not window then error("Failed to create window") end
local renderer = app.create_renderer(window)
if not renderer then error("Failed to create renderer") end
local texture = app.create_texture(renderer, WIDTH, HEIGHT)
if not texture then error("Failed to create texture") end

app.configure({
    width = WIDTH,
    height = HEIGHT,
    title = "Lua Ray Tracing - Triangles",
    window = window,
    renderer = renderer,
    texture = texture
})

print("Initializing Embree from Lua...")
local device = EmbreeDevice.new()
local scene = device:create_scene()

-- Create scene: A single triangle
-- Vertices: (-0.5, -0.5, 0), (0.5, -0.5, 0), (0.0, 0.5, 0)
    print("Adding triangle...")
    scene:add_triangle(
        -0.5, -0.5, 0.0,
         0.5, -0.5, 0.0,
         0.0,  0.5, 0.0
    )
    
    scene:commit()

    print("Rendering scene...")

    local aspectRatio = WIDTH / HEIGHT
    local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707
    -- Re-normalize light direction
    local lightLen = math.sqrt(lightDirX*lightDirX + lightDirY*lightDirY + lightDirZ*lightDirZ)
    lightDirX, lightDirY, lightDirZ = lightDirX/lightLen, lightDirY/lightLen, lightDirZ/lightLen

    -- Lock once, write many pixels, then unlock
    local pixels, pitch = app.lock_texture(texture)
    if pixels ~= nil then
        for y = 0, HEIGHT - 1 do
            for x = 0, WIDTH - 1 do
                -- Normalize pixel coordinates [-1, 1]
                local u = (2.0 * x - WIDTH) / WIDTH
                local v = (2.0 * y - HEIGHT) / HEIGHT
                
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
                    
                    diffuse = 0.2 + 0.8 * diffuse
                    if diffuse > 1.0 then diffuse = 1.0 end
        
                    local shade = math.floor(255 * diffuse)
                    
                    -- Let's do a simple color using locked write API:
                    app.draw_pixel_locked(pixels, pitch, x, y, math.floor(shade * 1.0), math.floor(shade * 0.8), math.floor(shade * 0.8))
                else
                    app.draw_pixel_locked(pixels, pitch, x, y, 50, 50, 60) -- Dark blue-ish background
                end
            end
        end
        app.unlock_texture(texture)
    end

print("Lua render finished.")

function app.on_frame()
    -- Optional: Add GUI or animation updates here
end
