local BSDF = {}
local PI = math.pi
local EPSILON = 1e-7

local function clamp(x, low, high) return math.max(low, math.min(high, x)) end
local function dot(a, b) return a[1]*b[1] + a[2]*b[2] + a[3]*b[3] end
local function normalize(v)
    local length = math.sqrt(math.max(dot(v, v), EPSILON))
    return {v[1]/length, v[2]/length, v[3]/length}
end
local function cross(a, b)
    return {a[2]*b[3]-a[3]*b[2], a[3]*b[1]-a[1]*b[3], a[1]*b[2]-a[2]*b[1]}
end
local function basis(n)
    local helper = math.abs(n[3]) < 0.999 and {0,0,1} or {0,1,0}
    local tangent = normalize(cross(helper, n))
    return tangent, cross(n, tangent)
end
local function world(local_direction, n)
    local tangent, bitangent = basis(n)
    return normalize({
        tangent[1]*local_direction[1] + bitangent[1]*local_direction[2] + n[1]*local_direction[3],
        tangent[2]*local_direction[1] + bitangent[2]*local_direction[2] + n[2]*local_direction[3],
        tangent[3]*local_direction[1] + bitangent[3]*local_direction[2] + n[3]*local_direction[3]
    })
end
local function distribution(n_dot_h, roughness)
    local alpha = math.max(roughness * roughness, 0.0025)
    local alpha2 = alpha * alpha
    local denominator = n_dot_h*n_dot_h*(alpha2-1.0)+1.0
    return alpha2 / math.max(PI*denominator*denominator, EPSILON)
end
local function geometry_term(n_dot_x, roughness)
    local k = ((roughness+1.0)^2)/8.0
    return n_dot_x / math.max(n_dot_x*(1.0-k)+k, EPSILON)
end
local function specular_probability(material)
    return clamp(0.25 + 0.5 * clamp(material.metallic or 0, 0, 1), 0.25, 0.75)
end

function BSDF.evaluate(material, normal, wo, wi)
    normal, wo, wi = normalize(normal), normalize(wo), normalize(wi)
    local n_dot_o, n_dot_i = dot(normal, wo), dot(normal, wi)
    if n_dot_o <= 0 or n_dot_i <= 0 then return {0,0,0} end
    local half = normalize({wo[1]+wi[1], wo[2]+wi[2], wo[3]+wi[3]})
    local n_dot_h, o_dot_h = math.max(dot(normal, half), 0), math.max(dot(wo, half), 0)
    local metallic = clamp(material.metallic or 0, 0, 1)
    local roughness = clamp(material.roughness or 0.5, 0.045, 1)
    local base = material.base_color or {1,1,1}
    local d = distribution(n_dot_h, roughness)
    local g = geometry_term(n_dot_o, roughness) * geometry_term(n_dot_i, roughness)
    local result = {}
    for i=1,3 do
        local f0 = 0.04*(1-metallic) + base[i]*metallic
        local fresnel = f0 + (1-f0)*(1-o_dot_h)^5
        local specular = d*g*fresnel / math.max(4*n_dot_o*n_dot_i, EPSILON)
        local diffuse = (1-fresnel)*(1-metallic)*base[i]/PI
        result[i] = math.max(0, diffuse+specular)
    end
    return result
end

function BSDF.pdf(material, normal, wo, wi)
    normal, wo, wi = normalize(normal), normalize(wo), normalize(wi)
    local n_dot_o, n_dot_i = dot(normal, wo), dot(normal, wi)
    if n_dot_o <= 0 or n_dot_i <= 0 then return 0 end
    local half = normalize({wo[1]+wi[1], wo[2]+wi[2], wo[3]+wi[3]})
    local n_dot_h, o_dot_h = math.max(dot(normal, half), 0), math.max(dot(wo, half), EPSILON)
    local specular_pdf = distribution(n_dot_h, clamp(material.roughness or 0.5, 0.045, 1))*n_dot_h/(4*o_dot_h)
    local probability = specular_probability(material)
    return probability*specular_pdf + (1-probability)*n_dot_i/PI
end

function BSDF.sample(material, normal, wo, rng)
    normal, wo = normalize(normal), normalize(wo)
    local wi
    if rng:next() < specular_probability(material) then
        local roughness = clamp(material.roughness or 0.5, 0.045, 1)
        local alpha = math.max(roughness*roughness, 0.0025)
        local u1, u2 = rng:next(), rng:next()
        local phi = 2*PI*u1
        local cos_theta = math.sqrt((1-u2)/(1+(alpha*alpha-1)*u2))
        local sin_theta = math.sqrt(math.max(0, 1-cos_theta*cos_theta))
        local half = world({sin_theta*math.cos(phi), sin_theta*math.sin(phi), cos_theta}, normal)
        local scale = 2*dot(wo, half)
        wi = normalize({scale*half[1]-wo[1], scale*half[2]-wo[2], scale*half[3]-wo[3]})
    end
    if not wi or dot(normal, wi) <= 0 then
        local u1, u2 = rng:next(), rng:next()
        local radius, phi = math.sqrt(u1), 2*PI*u2
        wi = world({radius*math.cos(phi), radius*math.sin(phi), math.sqrt(1-u1)}, normal)
    end
    local pdf = BSDF.pdf(material, normal, wo, wi)
    return wi, BSDF.evaluate(material, normal, wo, wi), pdf, false
end

function BSDF.power_heuristic(pdf_a, pdf_b)
    local a, b = pdf_a*pdf_a, pdf_b*pdf_b
    if a+b == 0 then return 0 end
    return a/(a+b)
end

return BSDF
