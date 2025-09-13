# clipyank

A simple utility to sync your Vim yank register with the system clipboard.

## Dependencies

This tool requires external clipboard utilities to be installed on your system. Please install the appropriate one for your display server.

### For X11

- `xclip`

**Installation (Debian/Ubuntu):**

```bash
sudo apt-get update
sudo apt-get install xclip
```

**Installation (Fedora):**

```bash
sudo dnf install xclip
```

**Installation (Arch):**

```bash
sudo pacman -Syy
sudo pacman -S xclip
```

### For Wayland

- `wl-clipboard` (which provides `wl-copy` and `wl-paste`)

**Installation (Debian/Ubuntu):**

```bash
sudo apt-get update
sudo apt-get install wl-clipboard
```

**Installation (Fedora):**

```bash
sudo dnf install wl-clipboard
```

**Installation (Arch):**

```bash
sudo pacman -Syy
sudo pacman -S wl-clipboard
```

## Build

```bash
make
```

## Installation

To install the `clipyank` executable to `/usr/local/bin` and create history file, run:

```bash
make install
```

## Unsinstallation

To remove the `clipyank` executable and the history file, run:

```bash
make uninstall
```

## Usage

### Vim Integration

Add the following to your `.vimrc`:

```vim
" Send yanked text to the system clipboard (Visual mode)
" Press `,cp` to copy (considering <leader> = ',')
vnoremap <leader>cp :!clipyank --send<CR>

" Paste from the system clipboard (Normal mode)
" Press `,cv` to paste (considering <leader> = ',')
nnoremap <leader>cv :r !clipyank --recv<CR>

" Show clip history from clipyank (Normal mode)
" Press `,show` to paste (considering <leader> = ',')
nnoremap <leader>show :w !clipyank --recv<CR>
```

1. Add this code to your ~/.vimrc file.
2. Restart Vim or run :so ~/.vimrc in Normal mode.
3. To copy: Select text in Visual mode and press \cp (or your custom leader key + cp).
4. To paste: In Normal mode, press \cv to paste from the system clipboard.
5. To show history: In Normal mode, press \show to show clip history.

> To know your leader key, execute `:let mapleader` in vim normal mode. If it gives an error, no key is set and so default is (`\`).

### Command Line

**Show History:** `clipyank --show`
**Send from file:** `cat my_file.txt | clipyank --send`
**Send text to clipboard:** `echo "random text" | clipyank --send`
**Receive to file:** `clipyank --recv > my_file.txt`

> You can always set shortcuts to the above mentioned commands in your shell config files like `.bashrc` or `.zshrc`.
