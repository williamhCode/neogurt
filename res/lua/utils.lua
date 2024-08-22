local M = {}

--- Converts command arguments to a table
--- @param opts table
--- @return string|nil, table
M.convert_command_args = function(opts)
  local args = opts.fargs
  if #args == 0 then
    vim.api.nvim_err_writeln("No arguments provided. Expected at least one argument.")
    return nil, {}
  end

  local cmd = table.remove(args, 1)
  local cmd_opts = {}
  for _, arg in ipairs(args) do
    local key, value = arg:match("([^=]+)=(.*)")
    if not key or not value then
      vim.api.nvim_err_writeln("Invalid argument format: " .. arg .. ". Expected format is key=value.")
      return nil, {}
    end
    cmd_opts[key] = value
  end

  return cmd, cmd_opts
end

--- Converts a string to a boolean
--- @param value any
--- @return boolean?
local tobool = function(value)
  if not (value == "false" or value == "true") then
    return nil
  end
  return value == "true"
end

local all_types = {
  "nil",
  "boolean",
  "number",
  "string",
  "userdata",
  "function",
  "thread",
  "table"
}

M.convert_opts = function(default_opts, opts)
  for key, default_val in pairs(default_opts) do
    local expected_type
    if vim.tbl_contains(all_types, default_val) then
      expected_type = default_val
    else
      expected_type = type(default_val)
    end

    if opts[key] ~= nil then
      local new_val
      if expected_type == "boolean" then
        new_val = tobool(opts[key])
      elseif expected_type == "number" then
        new_val = tonumber(opts[key])
      elseif expected_type == "string" then
        new_val = opts[key]
      end

      if new_val == nil then
        local msg = string.format("Invalid value for key '%s'. Expected type '%s'.", key, expected_type)
        vim.api.nvim_err_writeln(msg)
        return false
      end

      opts[key] = new_val
    end
  end
  return true
end

M.get_neogui_channel = function()
  local uis = vim.api.nvim_list_uis()
  for _, ui in ipairs(uis) do
    local client = vim.api.nvim_get_chan_info(ui.chan).client
    if client and client.name == "neogui" and client.type == "ui" then
      return ui.chan
    end
  end

  vim.api.nvim_err_writeln("Cannot find neogui client")
  return nil
end

M.merge_opts = function(default_opts, opts)
  local new_opts = {}

  for key, default_val in pairs(default_opts) do
    local val = opts[key]
    local expected_type

    -- if required option
    if vim.tbl_contains(all_types, default_val) then
      if val == nil then
        local msg = string.format("Key '%s' with type '%s' is required", key, default_val)
        vim.api.nvim_err_writeln(msg)
        return nil
      end
      expected_type = default_val
    else
      expected_type = type(default_val)
    end

    if val ~= nil then
      if type(opts[key]) ~= expected_type then
        local msg = string.format("Invalid value for key '%s'. Expected type '%s'.", key, expected_type)
        vim.api.nvim_err_writeln(msg)
        return nil
      end
      new_opts[key] = val
    else
      new_opts[key] = default_val
    end
  end

  if next(new_opts) == nil then
    -- _ is a placeholder so rpc request sends empty table not array
    new_opts = { _ = nil }
  end

  return new_opts
end

return M
