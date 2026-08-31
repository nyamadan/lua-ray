local RNG = {}
local UINT32 = 0xffffffff
local SCALE = 1.0 / 4294967296.0

local function mix(value)
    value = (value ~ (value >> 16)) & UINT32
    value = (value * 0x7feb352d) & UINT32
    value = (value ~ (value >> 15)) & UINT32
    value = (value * 0x846ca68b) & UINT32
    return (value ~ (value >> 16)) & UINT32
end

function RNG.for_pixel(x, y, pass)
    local state = mix((x * 0x1f123bb5 ~ y * 0x5f356495 ~ pass * 0x6c8e9cf5) & UINT32)
    if state == 0 then state = 0x9e3779b9 end
    local rng = {}
    function rng:next()
        state = (state ~ ((state << 13) & UINT32)) & UINT32
        state = (state ~ (state >> 17)) & UINT32
        state = (state ~ ((state << 5) & UINT32)) & UINT32
        return state * SCALE
    end
    return rng
end

return RNG
