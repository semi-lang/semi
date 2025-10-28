# Editor Configuration

# Formatting

This project uses clang-format for code formatting.

# Folding

Regions are defined using `#pragma region <RegionName>` and `#pragma endregion`. For VIM users, you can use the following settings to recognize these regions:

```vim
set foldmethod=marker
set foldmarker=#pragma\ region,#pragma\ endregion
```