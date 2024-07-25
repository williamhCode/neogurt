vim.g.neogui = true
vim.g.neogui_opts = {
  window = {
    vsync = true,
    high_dpi = true,
    borderless = false,
    blur = 0,
  },
  margins = {
    top = 0,
    bottom = 0,
    left = 0,
    right = 0,
  },
  multigrid = true,
  mac_opt_is_meta = true,
  cursor_idle_time = 10,
  scroll_speed = 1,
  bg_color = 0x000000,
  transparency = 1,
  max_fps = 120,
}

vim.g.set_neogui_opts = function(opts)
  vim.g.neogui_opts = vim.tbl_deep_extend("force", vim.g.neogui_opts, opts)
end

---Converts command arguments to a table
---@param opts table
---@return string|nil, table|nil
local convert_command_args = function(opts)
  local args = opts.fargs
  if #args == 0 then
    vim.api.nvim_err_writeln("No arguments provided. Expected at least one argument.")
    return nil, nil
  end

  local cmd = table.remove(args, 1)
  -- placeholder so rpc request is sends empty table not array
  local cmd_opts = { placeholder = "" }
  for _, arg in ipairs(args) do
    local key, value = arg:match("([^=]+)=(.*)")
    if not key or not value then
      vim.api.nvim_err_writeln("Invalid argument format: " .. arg .. ". Expected format is key=value.")
      return nil, nil
    end
    cmd_opts[key] = value
  end

  return cmd, cmd_opts
end

---Sends a session command to all neogui clients.
---@param cmd string: The command to send.
---@param opts? table command options
vim.g.neogui_session = function(cmd, opts)
  local uis = vim.api.nvim_list_uis()
  for _, ui in ipairs(uis) do
    local chan_id = ui.chan
    local client = vim.api.nvim_get_chan_info(chan_id).client
    if client and client.name == "neogui" and client.type == "ui" then
      local response = vim.rpcrequest(chan_id, "neogui_session", cmd, opts)
      vim.print(response)
      -- if errMsg ~= nil then
      --   vim.api.nvim_err_writeln("NeoguiSession error: " .. errMsg)
      -- end
    end
  end
end

vim.api.nvim_create_user_command("NeoguiSession", function(opts)
  local cmd, cmd_opts = convert_command_args(opts)
  if not cmd or not cmd_opts then return end
  vim.g.neogui_session(cmd, cmd_opts)
end, { nargs = "*" })
