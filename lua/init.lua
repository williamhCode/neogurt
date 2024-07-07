vim.g.neogui = 1
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

vim.api.nvim_create_user_command("NeoguiSession", function(opts)
  local uis = vim.api.nvim_list_uis()
  for _, ui in ipairs(uis) do
    local chan_id = ui.chan
    local client = vim.api.nvim_get_chan_info(chan_id).client
    if client and client.name == "neogui" and client.type == "ui" then
      vim.rpcnotify(chan_id, "session_cmd", unpack(opts.fargs))
    end
  end
end, { nargs = "*" })
