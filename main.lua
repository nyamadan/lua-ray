-- SDL3 Ray Tracing in Lua

-- RayTracer Class Definition
RayTracer = {}
RayTracer.__index = RayTracer

function RayTracer.new(width, height)
    local self = setmetatable({}, RayTracer)
    self.width = width
    self.height = height
    self.window = nil
    self.renderer = nil
    self.texture = nil
    self.device = nil
    self.scene = nil
    return self
end

function RayTracer:init()
    -- 1. Init Video
    if not app.init_video() then
        error("Error: Failed to init video")
    end

    -- 2. Create Window
    self.window = app.create_window(self.width, self.height, "Lua Ray Tracing (OO Refactor)")
    if self.window == nil then
        error("Error: Failed to create window")
    end

    -- 3. Create Renderer
    self.renderer = app.create_renderer(self.window)
    if self.renderer == nil then
        error("Error: Failed to create renderer")
    end

    -- 4. Create Texture
    self.texture = app.create_texture(self.renderer, self.width, self.height)
    if self.texture == nil then
        error("Error: Failed to create texture")
    end

    -- 5. Create AppData
    self.data = AppData.new(self.width, self.height)

    -- 6. Configure App (Dependency Injection for C++)
    app.configure({
        width = self.width,
        height = self.height,
        title = "Lua Ray Tracing (AppData)",
        window = self.window,
        renderer = self.renderer,
        texture = self.texture
    })
end

function RayTracer:init_scene()
    print("Initializing Embree from Lua...")
    self.device = EmbreeDevice.new()
    self.scene = self.device:create_scene()

    -- Create scene: Sphere at (0, 0, 0) with radius 0.5
    self.scene:add_sphere(0.0, 0.0, 0.0, 0.5)
    self.scene:commit()
end

function RayTracer:render()
    print("Rendering scene...")

    local aspectRatio = self.width / self.height
    local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707
    
    -- Render to AppData
    for y = 0, self.height - 1 do
        for x = 0, self.width - 1 do
            -- Normalize pixel coordinates [-1, 1]
            local u = (2.0 * x - self.width) / self.width
            local v = (2.0 * y - self.height) / self.height
            
            u = u * aspectRatio

            -- Ray setup (Camera at 0, 0, 1 looking at -z)
            local ox, oy, oz = 0.0, 0.0, 1.0
            local dx, dy, dz = u, v, -1.0

            -- Normalize direction
            local len = math.sqrt(dx*dx + dy*dy + dz*dz)
            dx, dy, dz = dx/len, dy/len, dz/len

            -- Intersect
            local hit, t, nx, ny, nz = self.scene:intersect(ox, oy, oz, dx, dy, dz)

            if hit then
                -- Diffuse shading
                local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
                if diffuse < 0 then diffuse = 0 end
                local shade = math.floor(255 * diffuse)
                self.data:set_pixel(x, y, shade, shade, shade)
            else
                self.data:set_pixel(x, y, 128, 128, 128) -- Gray background
            end
        end
    end
    
    -- Update Texture from AppData
    app.update_texture(self.texture, self.data)
    
    print("Lua render finished.")
end

function RayTracer:on_ui()
    if ImGui.Begin("Lua Ray Tracer Control") then
        ImGui.Text("Welcome to Lua Ray Tracer!")
        ImGui.Text(string.format("Resolution: %d x %d", self.width, self.height))
        
        if ImGui.Button("Re-Render") then
            print("Re-rendering requested")
            self:render()
        end
    end
    ImGui.End()
end


-- Main Execution Flow
local WIDTH = 800
local HEIGHT = 600

-- Instantiate
raytracer = RayTracer.new(WIDTH, HEIGHT)

-- Initialize resources
raytracer:init()

-- Initialize scene
raytracer:init_scene()

-- Initial Render
raytracer:render()

-- Set global frame callback
function app.on_frame()
    raytracer:on_ui()
end
