#!/usr/bin/env python3
"""
Auto-generate node documentation from C++ source code.

This script parses SOPFactory::get_all_available_nodes() and individual
SOP headers to create comprehensive Markdown documentation for each node.

Usage:
    python tools/generate_node_docs.py

Output: docs/nodes/{category}/{node_name}.md
"""

import re
import os
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass


@dataclass
class NodeMetadata:
    """Metadata for a single node type"""
    type: str
    name: str
    category: str
    description: str


@dataclass
class Parameter:
    """Node parameter definition"""
    name: str
    type: str  # int, float, bool, string
    label: str
    default: Optional[str]
    range_min: Optional[str]
    range_max: Optional[str]
    options: List[str]  # For combo boxes
    category: str
    description: str


@dataclass
class InputConfig:
    """Node input configuration"""
    input_type: str  # NONE, SINGLE, MULTIPLE
    min_inputs: int
    max_inputs: int
    required: int


class NodeDocGenerator:
    """Generate documentation for Nodo nodes"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.nodo_core = project_root / "nodo_core"
        self.docs_dir = project_root / "docs"

    def parse_sop_factory(self) -> List[NodeMetadata]:
        """Parse SOPFactory::get_all_available_nodes() for node list"""
        factory_file = self.nodo_core / "src/sop/sop_factory.cpp"

        with open(factory_file, 'r') as f:
            content = f.read()

        # Find the get_all_available_nodes() function
        pattern = r'\{NodeType::(\w+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\}'
        matches = re.findall(pattern, content)

        nodes = []
        for match in matches:
            node_type, name, category, description = match
            nodes.append(NodeMetadata(
                type=node_type,
                name=name,
                category=category,
                description=description
            ))

        print(f"✓ Found {len(nodes)} nodes in SOPFactory")
        return nodes

    def parse_sop_header(self, node_type: str) -> Tuple[List[Parameter], InputConfig]:
        """Parse individual SOP header file for parameters and inputs"""
        # Convert NodeType to header filename (e.g., Sphere -> sphere_sop.hpp)
        header_name = f"{node_type.lower()}_sop.hpp"
        header_file = self.nodo_core / "include/nodo/sop" / header_name

        if not header_file.exists():
            print(f"  ⚠ Header not found: {header_file}")
            return [], InputConfig("UNKNOWN", 0, 0, 0)

        with open(header_file, 'r') as f:
            content = f.read()

        parameters = self._extract_parameters(content)
        input_config = self._extract_input_config(content)

        return parameters, input_config

    def _extract_parameters(self, content: str) -> List[Parameter]:
        """Extract parameter definitions from register_parameter() calls"""
        parameters = []

        # Pattern for parameter registration
        # Example: define_float_parameter("radius", DEFAULT_RADIUS)
        #            .label("Radius")
        #            .range(0.01, 100.0)
        #            .category("Size")
        #            .description("...")

        # Find all register_parameter blocks
        param_blocks = re.findall(
            r'register_parameter\((.*?)\);',
            content,
            re.DOTALL
        )

        for block in param_blocks:
            param = self._parse_parameter_block(block)
            if param:
                parameters.append(param)

        return parameters

    def _parse_parameter_block(self, block: str) -> Optional[Parameter]:
        """Parse a single parameter definition block"""
        # Extract type and name
        type_match = re.search(r'define_(int|float|bool|string)_parameter\("([^"]+)",\s*([^)]+)\)', block)
        if not type_match:
            return None

        param_type, param_name, default_value = type_match.groups()

        # Extract label
        label_match = re.search(r'\.label\("([^"]+)"\)', block)
        label = label_match.group(1) if label_match else param_name.title()

        # Extract range
        range_match = re.search(r'\.range\(([^,]+),\s*([^)]+)\)', block)
        range_min = range_match.group(1).strip() if range_match else None
        range_max = range_match.group(2).strip() if range_match else None

        # Extract options (for combo boxes)
        options_match = re.search(r'\.options\(\{([^}]+)\}\)', block)
        options = []
        if options_match:
            options_str = options_match.group(1)
            options = [opt.strip().strip('"') for opt in options_str.split(',')]

        # Extract category
        category_match = re.search(r'\.category\("([^"]+)"\)', block)
        category = category_match.group(1) if category_match else "General"

        # Extract description
        desc_match = re.search(r'\.description\("([^"]+)"\)', block)
        description = desc_match.group(1) if desc_match else ""

        return Parameter(
            name=param_name,
            type=param_type,
            label=label,
            default=default_value,
            range_min=range_min,
            range_max=range_max,
            options=options,
            category=category,
            description=description
        )

    def _extract_input_config(self, content: str) -> InputConfig:
        """Extract input configuration from get_input_config()"""
        config_match = re.search(
            r'InputConfig\(InputType::(\w+),\s*(\d+),\s*(\d+),\s*(\d+)\)',
            content
        )

        if config_match:
            input_type, min_in, max_in, required = config_match.groups()
            return InputConfig(input_type, int(min_in), int(max_in), int(required))

        # Default: single input modifier
        return InputConfig("SINGLE", 1, 1, 1)

    def generate_node_doc(self, node: NodeMetadata) -> str:
        """Generate Markdown documentation for a single node"""
        parameters, input_config = self.parse_sop_header(node.type)

        # Start building markdown
        md = []
        md.append(f"# {node.name}")
        md.append("")
        md.append(f"**Category:** {node.category}")
        md.append("")
        md.append("## Description")
        md.append("")
        md.append(node.description)
        md.append("")

        # Inputs section
        md.append("## Inputs")
        md.append("")
        if input_config.input_type == "NONE":
            md.append("This node generates geometry and requires no inputs.")
        elif input_config.input_type == "SINGLE":
            md.append(f"- **Input 0**: Geometry to process")
        elif input_config.input_type == "MULTIPLE":
            if input_config.max_inputs == -1:
                md.append(f"- **Inputs**: Accepts {input_config.min_inputs}+ geometry inputs")
            else:
                md.append(f"- **Inputs**: Accepts {input_config.min_inputs}-{input_config.max_inputs} geometry inputs")
        md.append("")

        # Parameters section
        if parameters:
            md.append("## Parameters")
            md.append("")

            # Group by category
            by_category: Dict[str, List[Parameter]] = {}
            for param in parameters:
                if param.category not in by_category:
                    by_category[param.category] = []
                by_category[param.category].append(param)

            for category, params in by_category.items():
                md.append(f"### {category}")
                md.append("")

                for param in params:
                    # Parameter name and type
                    md.append(f"**{param.label}** (`{param.type}`)")
                    md.append("")

                    # Description
                    if param.description:
                        md.append(param.description)
                        md.append("")

                    # Details
                    details = []
                    if param.default:
                        details.append(f"Default: `{param.default}`")
                    if param.range_min and param.range_max:
                        details.append(f"Range: `{param.range_min}` to `{param.range_max}`")
                    if param.options:
                        details.append(f"Options: {', '.join([f'`{opt}`' for opt in param.options])}")

                    if details:
                        md.append("- " + " | ".join(details))
                        md.append("")

        # Example usage section
        md.append("## Example Usage")
        md.append("")
        md.append(self._generate_example(node, parameters))
        md.append("")

        # See also section
        md.append("## See Also")
        md.append("")
        md.append(self._generate_see_also(node))
        md.append("")

        return "\n".join(md)

    def _generate_example(self, node: NodeMetadata, parameters: List[Parameter]) -> str:
        """Generate example usage text"""
        if node.category == "Generator":
            return f"Create a {node.name} node from the Node Library panel. Adjust parameters to customize the generated geometry."
        elif node.category == "Modifier":
            return f"Connect geometry to the input, then adjust parameters to modify the result."
        elif node.category == "Boolean":
            return f"Connect multiple geometry inputs to perform {node.name.lower()} operations."
        else:
            return f"Add a {node.name} node and connect appropriate inputs."

    def _generate_see_also(self, node: NodeMetadata) -> str:
        """Generate related nodes list"""
        related_map = {
            "Sphere": ["Box", "Cylinder", "Torus"],
            "Box": ["Sphere", "Cylinder", "Grid"],
            "Transform": ["Array", "Mirror", "Align"],
            "Extrude": ["PolyExtrude", "Bevel"],
            "Boolean": ["Merge", "Split"],
            "Group": ["Group Delete", "Group Combine", "Blast"],
        }

        related = related_map.get(node.name, [])
        if related:
            return "- " + "\n- ".join([f"[{name}]({name.lower().replace(' ', '_')}.md)" for name in related])
        else:
            return "*No related nodes*"

    def generate_all_docs(self):
        """Generate documentation for all nodes"""
        print("=== Nodo Node Documentation Generator ===\n")

        # Parse all nodes from factory
        nodes = self.parse_sop_factory()

        # Create docs directory structure
        for node in nodes:
            category_dir = self.docs_dir / "nodes" / node.category.lower()
            category_dir.mkdir(parents=True, exist_ok=True)

        # Generate docs for each node
        print("\nGenerating documentation:")
        for node in nodes:
            print(f"  • {node.name} ({node.category})")

            # Generate markdown
            markdown = self.generate_node_doc(node)

            # Write to file
            filename = node.name.lower().replace(" ", "_") + ".md"
            output_path = self.docs_dir / "nodes" / node.category.lower() / filename

            with open(output_path, 'w') as f:
                f.write(markdown)

        # Generate index page
        self._generate_node_index(nodes)

        print(f"\n✓ Generated {len(nodes)} node documentation pages")
        print(f"✓ Output: {self.docs_dir / 'nodes'}")

    def _generate_node_index(self, nodes: List[NodeMetadata]):
        """Generate nodes/index.md with all nodes categorized"""
        md = []
        md.append("# Node Reference")
        md.append("")
        md.append("Complete reference for all available nodes in Nodo.")
        md.append("")

        # Group by category
        by_category: Dict[str, List[NodeMetadata]] = {}
        for node in nodes:
            if node.category not in by_category:
                by_category[node.category] = []
            by_category[node.category].append(node)

        # Sort categories
        category_order = ["Generator", "Modifier", "Array", "Boolean", "Attribute", "Group", "IO", "Utility"]

        for category in category_order:
            if category not in by_category:
                continue

            md.append(f"## {category}")
            md.append("")

            for node in sorted(by_category[category], key=lambda n: n.name):
                link = f"{category.lower()}/{node.name.lower().replace(' ', '_')}.md"
                md.append(f"- **[{node.name}]({link})** - {node.description}")

            md.append("")

        output_path = self.docs_dir / "nodes" / "index.md"
        with open(output_path, 'w') as f:
            f.write("\n".join(md))

        print(f"  • Node index page")


def main():
    """Main entry point"""
    project_root = Path(__file__).parent.parent
    generator = NodeDocGenerator(project_root)
    generator.generate_all_docs()


if __name__ == "__main__":
    main()
