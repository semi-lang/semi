#!/usr/bin/env python3
# Copyright (c) 2025 Ian Chen
# SPDX-License-Identifier: MPL-2.0

"""
Amalgamation script for the Semi programming language.

This script combines multiple C and header files into two files:
1. semi.c - Contains all source code
2. semi.h - Contains header files

The script performs topological sort to ensure proper ordering of includes.
"""

import os
import re
from typing import Dict, List, Set, Tuple
from pathlib import Path

LOCAL_INCLUDE_PATTERN = re.compile(r'#include\s+"([^"]+)"')
IFNDEF_PATTERN = re.compile(r'#ifndef\s+([A-Z][A-Z0-9_]*_H)\s*$')

# Copyright lines to filter out when reading files
COPYRIGHT_LINES = [
    '// Copyright (c) 2025 Ian Chen',
    '// SPDX-License-Identifier: MPL-2.0',
]

def strip_include_guards(lines: List[str], file_path: str) -> List[str]:
    """
    Strip include guards from header files.
    
    Searches for header guards with the format:
    - #ifndef GUARD_NAME_H
    - #define GUARD_NAME_H
    - #endif (with optional comment)
    where GUARD_NAME_H is uppercase with underscores and ends with _H.
    
    Lines before #ifndef and after #endif are allowed (e.g., copyright comments).
    
    Args:
        lines: List of file lines (already stripped)
        file_path: Path to the file
        
    Returns:
        Lines without include guards
        
    Raises:
        ValueError: If no valid header guards are found
    """
    if not file_path.endswith('.h'):
        return lines
    
    # Find #ifndef with valid guard name
    ifndef_idx = None
    guard_name = None
    for i, line in enumerate(lines):
        ifndef_match = IFNDEF_PATTERN.match(line.strip())
        if ifndef_match:
            ifndef_idx = i
            guard_name = ifndef_match.group(1)
            break
    
    if ifndef_idx is None:
        raise ValueError(f"No valid header guard found in {file_path}: Expected '#ifndef GUARD_NAME_H' (uppercase with underscores, ending in _H)")
    
    # Check that the next line is the matching #define
    define_idx = ifndef_idx + 1
    if define_idx >= len(lines):
        raise ValueError(f"Invalid header guard in {file_path}: Missing #define after #ifndef")
    
    if not re.match(rf'#define\s+{re.escape(guard_name)}\s*$', lines[define_idx].strip()):
        raise ValueError(f"Invalid header guard in {file_path}: Expected '#define {guard_name}' on line after #ifndef, found '{lines[define_idx].strip()}'")
    
    # Find matching #endif (search backwards for first non-empty line)
    endif_idx = None
    for i in range(len(lines) - 1, define_idx, -1):
        stripped = lines[i].strip()
        if stripped:
            if stripped.startswith('#endif'):
                endif_idx = i
                break

            raise ValueError(f"Invalid header guard in {file_path}: Last non-empty line must start with '#endif', found '{stripped}'")
    
    if endif_idx is None:
        raise ValueError(f"Invalid header guard in {file_path}: Missing #endif for guard {guard_name}")
    
    # Return lines between #define and #endif (excluding both)
    return lines[define_idx + 1:endif_idx]

def remove_local_includes(lines: List[str]) -> List[str]:
    """
    Remove local #include directives (our own headers).
    
    Args:
        lines: List of file lines
        
    Returns:
        Lines without local includes
    """
    result = []
    for line in lines:
        # Keep the line if it's not a local include
        if not LOCAL_INCLUDE_PATTERN.match(line.strip()):
            result.append(line)
            
    return result

class FileNode:
    """Represents a source or header file with its dependencies."""
    
    def __init__(self, path: str):
        self.path = path
        self.dependencies: Set['FileNode'] = set()
        self.lines: List[str] = []
        self.is_header = path.endswith('.h')
        
    def __repr__(self):
        return f"FileNode({self.path})"


def read_file(file_path: str) -> FileNode:
    """
    Read a file and create a FileNode.
    
    Args:
        file_path: Path to the file
        
    Returns:
        FileNode containing file information (without resolved dependencies)
    """
    node = FileNode(file_path)
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            # Read, strip each line, and filter out copyright
            skip_copyright = True
            for line in f:
                line = line.rstrip()
                
                # Skip copyright lines at the beginning
                if skip_copyright:
                    if line in COPYRIGHT_LINES or line == '':
                        continue
                    else:
                        skip_copyright = False
                
                node.lines.append(line)
        print(f"Read file: {file_path}")
    except FileNotFoundError:
        print(f"Warning: File not found: {file_path}")
    
    return node


def topological_sort(nodes: List[FileNode]) -> List[FileNode]:
    """
    Perform topological sort on a list of FileNode objects based on their dependencies.
    
    Args:
        nodes: List of FileNode objects to sort
        
    Returns:
        List of FileNode objects in topologically sorted order (dependencies first)
    """
    visited = set()
    result = []
    
    def visit(node: FileNode):
        if node.path in visited:
            return
            
        visited.add(node.path)
        
        # Visit dependencies first
        for dep_node in node.dependencies:
            visit(dep_node)
            
        result.append(node)
    
    # Visit all provided nodes
    for node in nodes:
        visit(node)
    
    return result


class Amalgamator:
    """Amalgamates C source files and headers."""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include" / "semi"
        self.build_dir = self.project_root / "amalgamated"
        
        # Calculate paths for special header files
        self.header_files = [
            str(self.include_dir / 'config.h'),
            str(self.include_dir / 'error.h'),
            str(self.include_dir / 'semi.h'),
        ]
        
        # Map from file path to FileNode
        self.files: Dict[str, FileNode] = {}
        
        # Track which files have been processed
        self.processed: Set[str] = set()
        
    def normalize_path(self, include_path: str, current_file: str) -> str:
        """
        Normalize an include path to an absolute path.
        
        Args:
            include_path: The path from the #include directive
            current_file: The file containing the #include
            
        Returns:
            Normalized absolute path
        """
        current_dir = Path(current_file).parent
        
        # Handle relative includes from src/
        if include_path.startswith('./'):
            resolved = (current_dir / include_path[2:]).resolve()
        # Handle includes like "semi/config.h"
        elif include_path.startswith('semi/'):
            resolved = (self.include_dir / include_path[5:]).resolve()
        else:
            # Try to resolve from current directory first
            resolved = (current_dir / include_path).resolve()
            
        return str(resolved)
    
    def discover_files(self, start_files: List[str]):
        """
        Recursively discover all files.
        
        Reads each file, resolves its dependencies, and recursively processes
        all discovered dependencies.
        
        Args:
            start_file: List of path to be processed
        """
        to_process = [read_file(file) for file in start_files]
        self.files = { node.path: node for node in to_process }
        processed = set()

        while to_process:
            node = to_process.pop()
            if node in processed:
                continue
            
            # Find and resolve all local includes
            for line in node.lines:
                match = LOCAL_INCLUDE_PATTERN.match(line.strip())
                if not match:
                    continue
                    
                include_path = match.group(1)
                
                # Skip system includes
                if '<' in include_path or '>' in include_path:
                    continue
                
                # Resolve the include path
                resolved_path = self.normalize_path(include_path, node.path)
                
                # Skip if file doesn't exist
                if not os.path.exists(resolved_path):
                    continue
                
                # Skip special header files
                if resolved_path in self.header_files:
                    continue

                # Get the dependency node (will be filled when processed)
                if resolved_path in self.files:
                    dep_node = self.files[resolved_path]
                else:
                    # Create placeholder that will be replaced when processed
                    dep_node = read_file(resolved_path)
                    self.files[resolved_path] = dep_node
                
                # Add to this node's dependencies
                node.dependencies.add(dep_node)

                if node not in processed:
                    to_process.append(dep_node)
    
    def amalgamate(self) -> Tuple[str, str]:
        """
        Create amalgamated source and header files.
        
        Returns:
            Tuple of (source_content, header_content)
        """
        # Discover all .c files in src/ directory
        all_c_files = list(self.src_dir.glob("*.c"))
        
        # Discover dependencies for each .c file
        self.discover_files([str(file) for file in all_c_files])
        
        # Perform topological sort on all discovered files
        all_nodes = sorted(self.files.values(), key=lambda n: n.path)
        sorted_nodes = topological_sort(all_nodes)
        print('Topological sort order:')
        for node in sorted_nodes:
            print(f"  {node.path}")

        # Build the amalgamated source
        source_parts = []
        source_parts.append("// Copyright (c) 2025 Ian Chen")
        source_parts.append("// SPDX-License-Identifier: MPL-2.0")
        source_parts.append("")
        source_parts.append("// This is an amalgamated file containing all Semi language implementation.")
        source_parts.append("// Generated by amalgamate.py")
        source_parts.append("")
        source_parts.append('#include "semi.h"')
        source_parts.append("")

        # Separate headers and source files
        headers = [node for node in sorted_nodes if node.is_header]
        sources = [node for node in sorted_nodes if not node.is_header]
        
        # Add all headers first
        for node in headers:
            lines = strip_include_guards(node.lines, node.path)
            lines = remove_local_includes(lines)

            source_parts.append(f"// BEGIN: {os.path.basename(node.path)}")
            source_parts.extend(lines)
            source_parts.append(f"// END: {os.path.basename(node.path)}")
            source_parts.append("")
        
        # Add all source files
        for node in sources:
            lines = remove_local_includes(node.lines)

            source_parts.append(f"// BEGIN: {os.path.basename(node.path)}")
            source_parts.extend(lines)
            source_parts.append(f"// END: {os.path.basename(node.path)}")
            source_parts.append("")
        
        # Build the public header
        header_parts = []
        header_parts.append("// Copyright (c) 2025 Ian Chen")
        header_parts.append("// SPDX-License-Identifier: MPL-2.0")
        header_parts.append("")
        header_parts.append("// This is the public header for the Semi language.")
        header_parts.append("// Generated by amalgamate.py")
        header_parts.append("")
        header_parts.append("#ifndef SEMI_AMALGAMATED_H")
        header_parts.append("#define SEMI_AMALGAMATED_H")
        header_parts.append("")
        
        # Process special header files in order: config, error, semi
        for header_path in self.header_files:
            node = read_file(header_path)
            lines = strip_include_guards(node.lines, header_path)
            lines = remove_local_includes(lines)

            header_parts.append(f"// BEGIN: {os.path.basename(header_path)}")
            header_parts.extend(lines)
            header_parts.append(f"// END: {os.path.basename(header_path)}")
            header_parts.append("")
        
        header_parts.append("#endif /* SEMI_AMALGAMATED_H */")
        header_parts.append("")
        
        return '\n'.join(source_parts), '\n'.join(header_parts)
    
    def write_output(self, source_content: str, header_content: str):
        """
        Write the amalgamated files to the build directory.
        
        Args:
            source_content: Content for semi.c
            header_content: Content for semi.h
        """
        # Create build directory if it doesn't exist
        self.build_dir.mkdir(parents=True, exist_ok=True)
        
        # Write source file
        source_file = self.build_dir / "semi.c"
        with open(source_file, 'w', encoding='utf-8') as f:
            f.write(source_content)
        print(f"Written: {source_file}")
        
        # Write header file
        header_file = self.build_dir / "semi.h"
        with open(header_file, 'w', encoding='utf-8') as f:
            f.write(header_content)
        print(f"Written: {header_file}")


def main():
    # Get the project root (directory containing this script)
    script_dir = Path(__file__).parent
    
    amalgamator = Amalgamator(str(script_dir))
    source_content, header_content = amalgamator.amalgamate()
    amalgamator.write_output(source_content, header_content)
    
    print("Amalgamation complete!")


if __name__ == "__main__":
    main()
