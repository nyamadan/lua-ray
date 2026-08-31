local PBR = {}
local PI = math.pi
local EPSILON = 1e-5

local function clamp(value, low, high)
    return math.max(low, math.min(high, value))
end

function PBR.srgb_to_linear(value)
    value = clamp(value, 0.0, 1.0)
    if value <= 0.04045 then return value / 12.92 end
    return ((value + 0.055) / 1.055) ^ 2.4
end

function PBR.linear_to_srgb(value)
    value = math.max(0.0, value)
    if value <= 0.0031308 then return 12.92 * value end
    return 1.055 * value ^ (1.0 / 2.4) - 0.055
end

function PBR.aces_tonemap(value)
    local a, b, c, d, e = 2.51, 0.03, 2.43, 0.59, 0.14
    return clamp((value * (a * value + b)) / (value * (c * value + d) + e), 0.0, 1.0)
end

local function dot(a, b)
    return a[1] * b[1] + a[2] * b[2] + a[3] * b[3]
end

local function normalize(v)
    local length = math.sqrt(math.max(dot(v, v), EPSILON))
    return {v[1] / length, v[2] / length, v[3] / length}
end

local function fresnel_schlick(cos_theta, f0)
    local factor = (1.0 - clamp(cos_theta, 0.0, 1.0)) ^ 5
    return {
        f0[1] + (1.0 - f0[1]) * factor,
        f0[2] + (1.0 - f0[2]) * factor,
        f0[3] + (1.0 - f0[3]) * factor
    }
end

local function distribution_ggx(n_dot_h, roughness)
    local alpha = math.max(roughness * roughness, 0.0025)
    local alpha2 = alpha * alpha
    local denominator = n_dot_h * n_dot_h * (alpha2 - 1.0) + 1.0
    return alpha2 / math.max(PI * denominator * denominator, EPSILON)
end

local function geometry_schlick_ggx(n_dot_v, roughness)
    local k = ((roughness + 1.0) ^ 2) / 8.0
    return n_dot_v / math.max(n_dot_v * (1.0 - k) + k, EPSILON)
end

function PBR.evaluate_direct(base_color, metallic, roughness, normal, view, light, radiance)
    normal, view, light = normalize(normal), normalize(view), normalize(light)
    local half_vector = normalize({view[1] + light[1], view[2] + light[2], view[3] + light[3]})
    local n_dot_l = math.max(dot(normal, light), 0.0)
    local n_dot_v = math.max(dot(normal, view), 0.0)
    if n_dot_l <= 0.0 or n_dot_v <= 0.0 then return 0.0, 0.0, 0.0 end

    metallic = clamp(metallic, 0.0, 1.0)
    roughness = clamp(roughness, 0.045, 1.0)
    local f0 = {}
    for i = 1, 3 do f0[i] = 0.04 * (1.0 - metallic) + base_color[i] * metallic end
    local fresnel = fresnel_schlick(math.max(dot(half_vector, view), 0.0), f0)
    local distribution = distribution_ggx(math.max(dot(normal, half_vector), 0.0), roughness)
    local geometry = geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness)
    local denominator = math.max(4.0 * n_dot_v * n_dot_l, EPSILON)
    local result = {}
    for i = 1, 3 do
        local specular = distribution * geometry * fresnel[i] / denominator
        local diffuse = (1.0 - fresnel[i]) * (1.0 - metallic) * base_color[i] / PI
        result[i] = (diffuse + specular) * radiance[i] * n_dot_l
    end
    return result[1], result[2], result[3]
end

function PBR.to_display_byte(value)
    return math.floor(clamp(PBR.linear_to_srgb(PBR.aces_tonemap(value)), 0.0, 1.0) * 255.0 + 0.5)
end

return PBR
