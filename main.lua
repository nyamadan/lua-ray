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
    self.current_scene_type = "sphere" -- Default scene
    return self
end

function RayTracer:init()
    -- 1. Init Video
    if not app.init_video() then
        error("Error: Failed to init video")
    end

    -- 2. Create Window
    self.window = app.create_window(self.width, self.height, "Lua Ray Tracing")
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
    
    -- Initialize Embree Device once
    print("Initializing Embree Device...")
    self.device = EmbreeDevice.new()
end

function RayTracer:create_sphere_scene()
    print("Creating Sphere Scene...")
    -- Create scene: Sphere at (0, 0, 0) with radius 0.5
    self.scene:add_sphere(0.0, 0.0, 0.0, 0.5)
end

function RayTracer:create_triangle_scene()
    print("Creating Triangle Scene...")
    -- Create scene: A single triangle
    -- Vertices: (-0.5, -0.5, 0), (0.5, -0.5, 0), (0.0, 0.5, 0)
    self.scene:add_triangle(
        -0.5, -0.5, 0.0,
         0.5, -0.5, 0.0,
         0.0,  0.5, 0.0
    )
end

function RayTracer:create_color_pattern_scene()
    print("Creating Color Pattern Scene...")
    -- No objects, just background pattern
end

function RayTracer:reset_scene(scene_type)
    print("Resetting scene to: " .. scene_type)
    
    -- Explicitly release the old scene resources
    if self.scene then
        self.scene:release()
    end

    -- Clear current scene reference (Lua GC will handle the C++ object destruction eventually, but we released resources manually)
    self.scene = nil
    collectgarbage() -- Optional: Suggest GC to run
    
    -- Create new empty scene
    self.scene = self.device:create_scene()
    
    self.current_scene_type = scene_type
    
    if scene_type == "sphere" then
        self:create_sphere_scene()
    elseif scene_type == "triangle" then
        self:create_triangle_scene()
    elseif scene_type == "color_pattern" then
        self:create_color_pattern_scene()
    else
        print("Unknown scene type, defaulting to sphere")
        self:create_sphere_scene()
    end
    
    self.scene:commit()
    -- Re-render immediately after switch
    self:render()
end

function RayTracer:init_scene()
    self:reset_scene(self.current_scene_type)
end

function RayTracer:render()
    print("Rendering scene...")

    local aspectRatio = self.width / self.height
    local lightDirX, lightDirY, lightDirZ = 0.707, 0.0, 0.707
    -- Normalize light
    local len = math.sqrt(lightDirX*lightDirX + lightDirY*lightDirY + lightDirZ*lightDirZ)
    lightDirX, lightDirY, lightDirZ = lightDirX/len, lightDirY/len, lightDirZ/len
    
    -- Render to AppData
    -- 注意: テクスチャ書き込み時にY座標を上下反転している (flip_y = self.height - 1 - y)
    -- これは画像座標系（上が0）からテクスチャ座標系への変換のため
    for y = 0, self.height - 1 do
        -- Y座標を上下反転してテクスチャに書き込む
        local flip_y = self.height - 1 - y
        
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
                if self.current_scene_type == "color_pattern" then
                    -- Normal based coloring
                    -- Map normal [-1, 1] to [0, 1] -> [0, 255]
                    local r = math.floor((nx + 1.0) * 0.5 * 255)
                    local g = math.floor((ny + 1.0) * 0.5 * 255)
                    local b = math.floor((nz + 1.0) * 0.5 * 255)
                    -- 上下反転したY座標 (flip_y) を使用してピクセルを書き込む
                    self.data:set_pixel(x, flip_y, r, g, b)
                else
                    -- Diffuse shading
                    local diffuse = nx * lightDirX + ny * lightDirY + nz * lightDirZ
                    if diffuse < 0 then diffuse = 0 end
                    
                    -- Add some ambient light
                    diffuse = 0.2 + 0.8 * diffuse
                    if diffuse > 1.0 then diffuse = 1.0 end
                    
                    local shade = math.floor(255 * diffuse)
                    
                    if self.current_scene_type == "triangle" then
                         -- Yellowish tint for triangle
                         -- 上下反転したY座標 (flip_y) を使用してピクセルを書き込む
                        self.data:set_pixel(x, flip_y, math.floor(shade * 1.0), math.floor(shade * 0.8), math.floor(shade * 0.8))
                    else
                        -- White/Gray for sphere
                        -- 上下反転したY座標 (flip_y) を使用してピクセルを書き込む
                        self.data:set_pixel(x, flip_y, shade, shade, shade)
                    end
                end
            else
                if self.current_scene_type == "color_pattern" then
                    -- Background based on Ray Direction
                    local r = math.floor((dx + 1.0) * 0.5 * 255)
                    local g = math.floor((dy + 1.0) * 0.5 * 255)
                    local b = math.floor((dz + 1.0) * 0.5 * 255)
                    -- 上下反転したY座標 (flip_y) を使用してピクセルを書き込む
                    self.data:set_pixel(x, flip_y, r, g, b)
                else 
                    if self.current_scene_type == "triangle" then
                         -- 上下反転したY座標 (flip_y) を使用してピクセルを書き込む
                         self.data:set_pixel(x, flip_y, 50, 50, 60) -- Dark blue-ish background
                    else
                         -- 上下反転したY座標 (flip_y) を使用してピクセルを書き込む
                         self.data:set_pixel(x, flip_y, 128, 128, 128) -- Gray background
                    end
                end
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
        
        ImGui.Separator()
        
        ImGui.Text("Scene Selection:")
        local changed = false
        local type = self.current_scene_type
        
        if ImGui.RadioButton("Sphere", type == "sphere") then
            if type ~= "sphere" then
                self:reset_scene("sphere")
            end
        end
        ImGui.SameLine()
        if ImGui.RadioButton("Triangle", type == "triangle") then
            if type ~= "triangle" then
                self:reset_scene("triangle")
            end
        end
        ImGui.SameLine()
        if ImGui.RadioButton("Color Pattern", type == "color_pattern") then
            if type ~= "color_pattern" then
                self:reset_scene("color_pattern")
            end
        end
        
        ImGui.Separator()
        
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

-- Initial Render is called inside init_scene -> reset_scene, but we can call it here if we want strictly explicit
-- raytracer:render() -- reset_scene already calls render

-- Set global frame callback
function app.on_frame()
    raytracer:on_ui()
end
