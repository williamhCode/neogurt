vim.g.neogui = true

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
  session_switch = {
    id = "number",
  },
  session_prev = {},
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
    all = true,
    [1] = "number",
  },
  font_size_reset = {
    all = true,
  },
}

vim.api.nvim_create_user_command("Neogui", function(cmd_opts)
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

  -- handle command output
  local result = vim.g.neogui_cmd(cmd, opts)
  if cmd == "session_new" then
  elseif cmd == "session_kill" then
    if result == true then
      print("Session killed")
    else
      print("Session not found")
    end
  elseif cmd == "session_prev" then
    if result == false then
      print("No previous session")
    end
  elseif cmd == "session_list" then
    vim.print(result)
  end
end, { nargs = "*" })

--- Sends a session command to neogui
--- @param cmd string: command to send.
--- @param opts table: command options
--- @return any: result of the command
vim.g.neogui_cmd = function(cmd, opts)
  local chan_id = utils.get_neogui_channel()
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

    local id = vim.rpcrequest(chan_id, "neogui_cmd", cmd, opts)
    return id

  elseif cmd == "session_kill" then
    local success = vim.rpcrequest(chan_id, "neogui_cmd", cmd, opts)
    return success

  elseif cmd == "session_switch" then
    local success = vim.rpcrequest(chan_id, "neogui_cmd", cmd, opts)
    return success

  elseif cmd == "session_prev" then
    local success = vim.rpcrequest(chan_id, "neogui_cmd", cmd)
    return success

  elseif cmd == "session_list" then
    local list = vim.rpcrequest(chan_id, "neogui_cmd", cmd, opts)
    return list

  elseif cmd == "session_select" then
    local curr_id = vim.g.neogui_cmd("session_list", { sort = "time" })[1].id
    local list = vim.g.neogui_cmd("session_list", opts)
    vim.ui.select(list, {
      prompt = "Select a session",
      format_item = function(item)
        local line = "(" .. item.id .. ") - " .. item.name
        if item.id == curr_id then
          line = line .. " (current)"
        end
        return line
      end
    }, function(choice)
      if choice == nil then return end
      vim.g.neogui_cmd("session_switch", { id = choice.id })
    end)

  elseif cmd == "font_size_change" then
    return vim.rpcrequest(chan_id, "neogui_cmd", cmd, opts)

  elseif cmd == "font_size_reset" then
    return vim.rpcrequest(chan_id, "neogui_cmd", cmd, opts)
  end
end

vim.g.neogui_startup = function() end
