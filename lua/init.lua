vim.g.neogui = 1

vim.api.nvim_create_user_command("Session", function(opts)
  local uis = vim.api.nvim_list_uis()
  for _, ui in ipairs(uis) do
    local chan_id = ui.chan
    local client = vim.api.nvim_get_chan_info(chan_id).client
    if client and client.name == "neogui" and client.type == "ui" then
      vim.rpcnotify(chan_id, "session_cmd", unpack(opts.fargs))
    end
  end
end, { nargs = "*" })
