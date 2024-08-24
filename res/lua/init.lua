vim.g.neogui = true

local utils = require("utils")

-- if value is a type name, the option is required
local session_table = {
  new = {
    name = "",
    dir = "~/",
    switch_to = true,
  },
  kill = {
    id = 0
  },
  switch = {
    id = "number",
  },
  prev = {},
  list = {
    -- id, name, time (recency)
    sort = "id",
    reverse = false,
  },
  select = {
    -- forwarded to list opts
    sort = "id",
    reverse = false,
  }
}

vim.api.nvim_create_user_command("NeoguiSession", function(cmd_opts)
  local cmd, opts = utils.convert_command_args(cmd_opts)
  if cmd == nil then return end

  -- convert strings to correct types
  local default_opts = session_table[cmd]
  if default_opts == nil then
    vim.api.nvim_err_writeln("Invalid command: " .. cmd)
    return
  end

  if utils.convert_opts(default_opts, opts) == false then
    return
  end

  -- handle command output
  local result = vim.g.neogui_session(cmd, opts)
  if cmd == "new" then
  elseif cmd == "kill" then
    if result == true then
      print("Session killed")
    else
      print("Session not found")
    end
  elseif cmd == "prev" then
    if result == false then
      print("No previous session")
    end
  elseif cmd == "list" then
    vim.print(result)
  end

end, { nargs = "*" })

--- Sends a session command to neogui
--- @param cmd string: command to send.
--- @param opts table: command options
--- @return any: result of the command
vim.g.neogui_session = function(cmd, opts)
  local chan_id = utils.get_neogui_channel()
  if chan_id == nil then return end

  local default_opts = session_table[cmd]
  if default_opts == nil then
    vim.api.nvim_err_writeln("Invalid command: " .. cmd)
    return
  end
  -- merge + check required options and option types
  opts = opts or {}
  local opts = utils.merge_opts(default_opts, opts)
  if opts == nil then return end

  if cmd == "new" then
    opts.dir = vim.fn.fnamemodify(opts.dir, ":p")
    if vim.fn.isdirectory(opts.dir) == 0 then
      vim.api.nvim_err_writeln("Directory does not exist: " .. opts.dir)
      return
    end

    local id = vim.rpcrequest(chan_id, "neogui_session", cmd, opts)
    return id

  elseif cmd == "kill" then
    local success = vim.rpcrequest(chan_id, "neogui_session", cmd, opts)
    return success

  elseif cmd == "switch" then
    local success = vim.rpcrequest(chan_id, "neogui_session", cmd, opts)
    return success

  elseif cmd == "prev" then
    local success = vim.rpcrequest(chan_id, "neogui_session", cmd)
    return success

  elseif cmd == "list" then
    local list = vim.rpcrequest(chan_id, "neogui_session", cmd, opts)
    return list

  elseif cmd == "select" then
    local curr_id = vim.g.neogui_session("list", { sort = "time" })[1].id
    local list = vim.g.neogui_session("list", opts)
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
      vim.g.neogui_session("switch", { id = choice.id })
    end)
  end
end

vim.g.neogui_startup = function() end
