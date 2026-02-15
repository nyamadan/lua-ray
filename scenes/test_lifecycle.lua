local test_lifecycle = {}

function test_lifecycle.setup(scene, app_data)
    -- Do nothing
end

function test_lifecycle.start(scene, app_data)
    io.write("start called\n")
    io.flush()
end

function test_lifecycle.shade(app_data, x, y)
    -- Do nothing
end

function test_lifecycle.stop(scene)
    -- Write to standard output
    io.write("stop called\n")
    io.flush()
end

return test_lifecycle
