# Neogurt
Blazingly fast Neovim GUI written in C++23 and WebGPU.

<img width="840" alt="Neogurt screenshot" src="https://github.com/user-attachments/assets/6affed8f-88fe-4e4c-8698-9a543c0f9f57">

## Features
- Blazingly fast and energy efficient
- Smooth scrolling and cursor out of the box
- Session support, similar to tmux (not detachable yet, wip)
- Supported on macOS, Windows and Linux WIP

Known issues: https://github.com/williamhCode/neogurt/issues/3 \
Planned Features: https://github.com/williamhCode/neogurt/issues/2

## Getting Started

### Installation
Download the latest release at: \
https://github.com/williamhCode/neogurt/releases/latest

Or [build from source](https://github.com/williamhCode/neogurt?tab=readme-ov-file#build-instructions).

The software is still pre-release, so expect often changes to the code and API.

### Configuration

Below are the all of the default options:

```lua
-- check if nvim is launched by neogurt
if vim.g.neogurt then
  vim.g.neogurt_cmd("option_set", {
    -- global options (shared across all sessions)
    titlebar = "default", -- "default", "transparent", "none"
    show_title = true,
    blur = 0,
    gamma = 1.7,
    vsync = true,
    fps = 60,

    margin_top = 0,
    margin_bottom = 0,
    margin_left = 0,
    margin_right = 0,

    macos_option_is_meta = "none", -- "none", "only_left", "only_right", "both"
    cursor_idle_time = 10,
    scroll_speed = 1,

    -- session specific options
    bg_color = 0x000000, -- used when opacity < 1
    opacity = 1,
  })

  vim.o.guifont = "SF Mono:h15"
  vim.o.linespace = 0

  -- ime highlights set by Neogurt
  vim.api.nvim_set_hl(0, "NeogurtImeNormal", { link = "Normal" })
  vim.api.nvim_set_hl(0, "NeogurtImeSelected", { link = "Underlined" })
end
```

### Commands/API

Neogurt commands can be invoked in this format:
```vim
Neogurt cmd_name arg=value arg1 arg2
```

For example, to spawn a new session with the name `nvim config` and directory `~/.config/nvim`, do
```vim
Neogurt session_new name=nvim\ config dir=~/.config/nvim
```

The lua equivalent is to use the `neogurt_cmd` function:
```lua
--- @param cmd string: command to send.
--- @param opts table: command options
--- @return any: result of the command
vim.g.neogurt_cmd = function(cmd, opts)
  ...
end
```
For example, let's do the same thing with the lua api:
```lua
vim.g.neogurt_cmd("session_new", { name = "nvim config", dir = "~/.config/nvim" } )
```

Shown below are the default value of all options. \
Note that:
- `arg = value` are optional named arguments  
- `arg = "lua_type"` are required named arguments  
- `[i] = value` are optional positional arguments  
- `[i] = "lua_type"` are required positional arguments  
- `id = 0` is the current session id, just like vim buffers

```lua
local cmds_table = {
  -- sets neogurt options
  -- default argument values are specified above
  -- you can call this anytime, and the options will be updated dynamically
  option_set = {},

  -- returns session id
  session_new = {
    name = "",
    dir = "~/",
    switch_to = true, -- switch to session after creating it
  },
  -- returns success (bool)
  session_kill = {
    id = 0
  },
  -- returns new session id
  session_restart = {
    id = 0,
    curr_dir = false, -- use cwd instead of session dir
  },
  -- returns success (bool)
  session_switch = {
    id = "number"
  },
  -- switch to previous session
  -- returns success (bool)
  session_prev = {},
  -- returns session info table (id, name, dir)
  session_info = {
    id = 0
  },
  -- returns list of session info tables
  session_list = {
    sort = "id",  -- id, name, time (recency)
    reverse = false,
  },
  -- spawns a session picker using vim.ui.select()
  -- wrapper around session_list
  session_select = {
    sort = "id",
    reverse = false,
  },

  font_size_change = {
    [1] = "number",
    all = false,  -- change in all sessions
  },
  font_size_reset = {
    all = false,  -- change in all sessions
  },
}
```

Lastly, you can also run some code after starting up the application with
```lua
vim.g.neogurt_startup = function()
  ...
end
```

Below are some configurations I find useful:
```lua
if vim.g.neogurt then
  -- all modes
  local mode = {"", "!", "t", "l"};

  -- change font size
  map(mode, "<D-=>", "<cmd>Neogurt font_size_change 1 all=false<cr>")
  map(mode, "<D-->", "<cmd>Neogurt font_size_change -1 all=false<cr>")
  map(mode, "<D-0>", "<cmd>Neogurt font_size_reset all=false<cr>")

  -- session mappings
  map(mode, "<D-l>", "<cmd>Neogurt session_prev<cr>")
  map(mode, "<D-r>", "<cmd>Neogurt session_select sort=time<cr>")
  map(mode, "<D-R>", "<cmd>Neogurt session_restart<cr>")

  -- sessionizer (create or select session)
  local choose_session = function(startup)
    local curr_id = vim.g.neogurt_cmd("session_info").id
    local session_list = vim.g.neogurt_cmd("session_list", { sort = "time" })

    local cmd = [[
    echo "$({
      echo ~/;
      echo ~/.dotfiles;
      echo ~/.config/nvim; 
      echo ~/Documents/Notes;
      echo ~/Documents/Work/Resume stuff;
      find ~/Documents/Coding -mindepth 2 -maxdepth 2 -type d | sort -r;
    })"
    ]]
    local output = vim.fn.system(cmd)

    for dir in string.gmatch(output, "([^\n]+)") do
      table.insert(session_list, { dir = dir })
    end

    vim.ui.select(session_list, {
      prompt = "Sessions",
      format_item = function(session)
        if session.id ~= nil then
          if session.id == curr_id then
            return "* " .. session.name
          else
            return "- " .. session.name
          end
        else
          return session.dir
        end
      end
    }, function(choice)
      if choice == nil then return end

      if choice.id ~= nil then
        vim.g.neogurt_cmd("session_switch", { id = choice.id })
      else
        local fmod = vim.fn.fnamemodify
        local dir = fmod(choice.dir, ":p")
        local name = fmod(dir, ":h:h:t") .. "/" .. fmod(dir, ":h:t")

        if startup then
          vim.g.neogurt_cmd("session_new", { dir = dir, name = name })
          vim.g.neogurt_cmd("session_kill")
        else
          vim.g.neogurt_cmd("session_new", { dir = dir, name = name })
        end
      end
    end)
  end

  map(mode, "<D-f>", function()
    choose_session(false)
  end)

  vim.g.neogurt_startup = function()
    choose_session(true)
  end
end
```

## Build Instructions
```
git clone https://github.com/williamhCode/neogurt.git --recurse-submodules
git submodule update  # if submodules become out of sync
```

- Install the latest version of LLVM Clang
- Default generator is ninja

- Dependencies
  - Boost
  - Freetype
  - Harfbuzz
  - Others are bundled

Example on macOS:
```sh
brew install llvm
brew install ninja

brew install boost
brew install freetype
brew install harfbuzz
```

Building:
```sh
make build-setup
make build

make run  # runs executable

make package  # builds .dmg file in build/release
```
