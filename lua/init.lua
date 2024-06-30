vim.g.neogui = 1
vim.g.neogui_opts_default = {
  window = {
    vsync = true,
    highDpi = true,
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
  macOptIsMeta = true,
  cursorIdleTime = 10,
  bgColor = 0x000000,
  transparency = 1,
  maxFps = 0,
}
vim.g.resolve_neogui_opts = function()
  vim.g.neogui_opts_resolved = vim.tbl_deep_extend("force", vim.g.neogui_opts_default, vim.g.neogui_opts)
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
