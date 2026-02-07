-- lib/Material.lua
-- マテリアルシステム (Ray Tracing in One Weekend用)

local Vec3 = require('lib.Vec3')
local Ray = require('lib.Ray')

local Material = {}

-- ===========================================
-- Lambertian (拡散反射マテリアル)
-- ===========================================

function Material.Lambertian(albedo)
    local mat = {
        type = "lambertian",
        albedo = albedo
    }
    
    function mat:scatter(ray_in, rec)
        local scatter_direction = rec.normal + Vec3.random_unit_vector()
        
        -- 散乱方向がゼロに近い場合は法線方向を使用
        if scatter_direction:near_zero() then
            scatter_direction = rec.normal
        end
        
        local scattered = Ray.new(rec.p, scatter_direction)
        local attenuation = self.albedo
        return scattered, attenuation
    end
    
    return mat
end

-- ===========================================
-- Metal (鏡面反射マテリアル)
-- ===========================================

function Material.Metal(albedo, fuzz)
    local mat = {
        type = "metal",
        albedo = albedo,
        fuzz = fuzz < 1 and fuzz or 1
    }
    
    function mat:scatter(ray_in, rec)
        local reflected = Vec3.reflect(ray_in.direction:normalize(), rec.normal)
        
        -- fuzz (ぼかし) を適用
        local scattered_direction = reflected + Vec3.random_in_unit_sphere() * self.fuzz
        local scattered = Ray.new(rec.p, scattered_direction)
        local attenuation = self.albedo
        
        -- 反射レイが表面の下に入る場合は吸収（散乱なし）
        if Vec3.dot(scattered.direction, rec.normal) > 0 then
            return scattered, attenuation
        else
            return nil, nil
        end
    end
    
    return mat
end

-- ===========================================
-- Dielectric (誘電体/ガラスマテリアル)
-- ===========================================

-- シュリック近似（フレネル反射率）
local function reflectance(cosine, ref_idx)
    local r0 = (1 - ref_idx) / (1 + ref_idx)
    r0 = r0 * r0
    return r0 + (1 - r0) * ((1 - cosine) ^ 5)
end

function Material.Dielectric(index_of_refraction)
    local mat = {
        type = "dielectric",
        ir = index_of_refraction
    }
    
    function mat:scatter(ray_in, rec)
        local attenuation = Vec3.new(1.0, 1.0, 1.0)
        local refraction_ratio = rec.front_face and (1.0 / self.ir) or self.ir
        
        local unit_direction = ray_in.direction:normalize()
        local cos_theta = math.min(Vec3.dot(-unit_direction, rec.normal), 1.0)
        local sin_theta = math.sqrt(1.0 - cos_theta * cos_theta)
        
        local cannot_refract = refraction_ratio * sin_theta > 1.0
        local direction
        
        if cannot_refract or reflectance(cos_theta, refraction_ratio) > math.random() then
            -- 全反射またはシュリック近似による反射
            direction = Vec3.reflect(unit_direction, rec.normal)
        else
            -- 屈折
            direction = Vec3.refract(unit_direction, rec.normal, refraction_ratio)
        end
        
        local scattered = Ray.new(rec.p, direction)
        return scattered, attenuation
    end
    
    return mat
end

return Material
