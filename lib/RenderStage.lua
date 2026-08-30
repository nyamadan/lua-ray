local BlockUtils = require("lib.BlockUtils")

local RenderStage = {}

function RenderStage.setup_blocks(owner, queue_name)
    local blocks = BlockUtils.generate_blocks(
        owner.width, owner.height, owner.BLOCK_SIZE, 1
    )
    blocks = BlockUtils.shuffle_blocks(blocks)
    BlockUtils.setup_shared_queue(owner.data, blocks, queue_name)
end

function RenderStage.start_workers(owner, queue_name, script_path)
    RenderStage.setup_blocks(owner, queue_name)

    local workers = {}
    for thread_id = 0, owner.NUM_THREADS - 1 do
        local worker = ThreadWorker.create(
            owner.data,
            owner.scene,
            0,
            0,
            owner.width,
            owner.height,
            thread_id
        )
        worker:start(script_path, owner.current_scene_type)
        table.insert(workers, worker)
    end
    return workers
end

function RenderStage.all_workers_done(workers)
    for _, worker in ipairs(workers) do
        if not worker:is_done() then
            return false
        end
    end
    return true
end

function RenderStage.create_coroutine(owner, queue_name, pixel_callback, on_complete, on_start)
    local WorkerUtils = require("workers.worker_utils")
    return coroutine.create(function()
        if on_start then
            on_start()
        end
        RenderStage.setup_blocks(owner, queue_name)

        local function check_cancel()
            coroutine.yield()
            return false
        end

        local function on_block_complete()
            coroutine.yield()
        end

        WorkerUtils.process_blocks(
            owner.data,
            queue_name,
            queue_name .. "_idx",
            pixel_callback,
            check_cancel,
            nil,
            on_block_complete
        )

        if on_complete then
            on_complete()
        end
    end)
end

return RenderStage
