vim.g.neogui = true

local utils = require("utils")

-- if value is a type name, the option is required
local session_table = {
  new = {
    name = "",
    dir = "~/",
    switch_to = true,
  },
  prev = {},
  switch = {
    id = "number",
  },
  list = {
    -- id, name, time (recency)
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

  return vim.g.neogui_session(cmd, opts)

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
  local opts = utils.merge_opts(default_opts, opts)
  if opts == nil then return end

  if cmd == "new" then
    opts.dir = vim.fn.fnamemodify(opts.dir, ":p")
    if vim.fn.isdirectory(opts.dir) == 0 then
      vim.api.nvim_err_writeln("Directory does not exist: " .. opts.dir)
      return
    end

    return vim.rpcrequest(chan_id, "neogui_session", cmd, opts)

  elseif cmd == "prev" then
    local success = vim.rpcrequest(chan_id, "neogui_session", cmd)
    if not success then
      vim.print("No previous session")
    end
    return success

  elseif cmd == "switch" then
    return vim.rpcrequest(chan_id, "neogui_session", cmd, opts)

  elseif cmd == "list" then
    return vim.rpcrequest(chan_id, "neogui_session", cmd, opts)
  end
end
