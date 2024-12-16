-- vim.api.nvim_command("autocmd VimEnter * call rpcrequest(1, 'vimenter')")
-- vim.api.nvim_command("autocmd UIEnter * call rpcrequest(1, 'uienter')")

vim.g.neogurt = true
vim.g.neogurt_opts = {}
vim.g.neogurt_startup = function() end

local utils = require("utils")

-- if value is a type name, the option is required
local cmds_table = {
  session_new = {
    name = "",
    dir = "~/",
    switch_to = true,
  },
  session_kill = {
    id = 0
  },
  session_restart = {
    id = 0,
    curr_dir = false,
  },
  session_switch = {
    id = "number",
  },
  session_prev = {},
  session_info = {
    id = 0
  },
  session_list = {
    -- id, name, time (recency)
    sort = "id",
    reverse = false,
  },
  session_select = {
    -- forwarded to list opts
    sort = "id",
    reverse = false,
  },

  font_size_change = {
    [1] = "number",
    all = false,
  },
  font_size_reset = {
    all = false,
  },
}

vim.api.nvim_create_user_command("Neogurt", function(cmd_opts)
  local cmd, opts = utils.convert_command_args(cmd_opts)
  if cmd == nil then return end

  -- convert strings to correct types
  local default_opts = cmds_table[cmd]
  if default_opts == nil then
    vim.api.nvim_err_writeln("Invalid command: " .. cmd)
    return
  end

  if utils.convert_opts(default_opts, opts) == false then
    return
  end

  -- print command output
  local result = vim.g.neogurt_cmd(cmd, opts)
  vim.print(result)

end, { nargs = "*" })

--- Sends a session command to neogurt
--- @param cmd string: command to send.
--- @param opts table: command options
--- @return any: result of the command
vim.g.neogurt_cmd = function(cmd, opts)
  local chan_id = utils.get_neogurt_channel()
  if chan_id == nil then return end

  local default_opts = cmds_table[cmd]
  if default_opts == nil then
    vim.api.nvim_err_writeln("Invalid command: " .. cmd)
    return
  end
  -- merge + check required options and option types
  opts = opts or {}
  local opts = utils.merge_opts(default_opts, opts)
  if opts == nil then return end

  -- convert positional keys to named keys (arg1, arg1...)
  -- cuz rpc can't mix table and array
  opts = utils.convert_positional_keys(opts)

  if cmd == "session_new" then
    opts.dir = vim.fn.fnamemodify(opts.dir, ":p")
    if vim.fn.isdirectory(opts.dir) == 0 then
      vim.api.nvim_err_writeln("Directory does not exist: " .. opts.dir)
      return
    end

  elseif cmd == "session_select" then
    local curr_id = vim.g.neogurt_cmd("session_list", { sort = "time" })[1].id
    local list = vim.g.neogurt_cmd("session_list", opts)
    vim.ui.select(list, {
      prompt = "Select a session",
      format_item = function(item)
        local line = item.name
        if item.id == curr_id then
          line = line .. " (current)"
        end
        return line
      end
    }, function(choice)
      if choice == nil then return end
      vim.g.neogurt_cmd("session_switch", { id = choice.id })
    end)
    return
  end

  if next(opts) == nil then
    return vim.rpcrequest(chan_id, "neogurt_cmd", cmd)
  else
    return vim.rpcrequest(chan_id, "neogurt_cmd", cmd, opts)
  end
end
