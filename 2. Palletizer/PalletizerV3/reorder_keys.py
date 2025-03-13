import argparse
import os
import yaml


def reorder_keys(data):
    """
    Reorders the keys in each dictionary within the 'rows' list of the YAML data.

    The keys in each dictionary are rearranged to follow the sequence:
    x, y, z, t, g.

    Parameters:
        data (dict): The input YAML data loaded as a dictionary. It must contain a key 'rows'
                     which is a list of dictionaries.

    Returns:
        dict: The modified data with each dictionary in 'rows' reordered to have keys in the
              order of x, y, z, t, g. If a key from the desired order is not found in a dictionary,
              it is simply skipped.
    """
    desired_order = ['x', 'y', 'z', 't', 'g']
    new_rows = []
    for item in data.get('rows', []):
        new_item = {key: item[key] for key in desired_order if key in item}
        new_rows.append(new_item)
    data['rows'] = new_rows
    return data


def main():
    """
    Main function to process a YAML file and reorder keys in the 'rows' section.

    This function parses command-line arguments to obtain the input YAML file name and an optional
    output file name. If the output file name is not provided, it automatically generates one based on
    the input file name. The YAML file is read, processed to reorder keys within each dictionary in the
    'rows' list, and then written back to the output file.

    Command-line Arguments:
        --input:  (Required) The name of the input YAML file.
        --output: (Optional) The name of the output YAML file. If not provided, the output file is named
                  using the format '<input_filename>_out<original_extension>'.

    Example:
        To explicitly specify the output file:
            python reorder_keys.py --input test.yaml --output test_out.yaml
        Or, to generate the output file name automatically:
            python reorder_keys.py --input test.yaml

    Dependencies:
        - PyYAML (Install via pip: pip install PyYAML)
    """
    parser = argparse.ArgumentParser(
        description='Reorder YAML keys in each entry of the "rows" list to the order: x, y, z, t, g.',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument('--input', required=True, help='Input YAML file name (required)')
    parser.add_argument('--output', required=False, help='Output YAML file name (optional).\n'
                        'If not provided, the output file will be named as\n'
                        '"<input_filename>_out<original_extension>"')
    args = parser.parse_args()

    input_file = args.input
    output_file = args.output

    # If the output file name is not provided, create a default output file name.
    if not output_file:
        base, ext = os.path.splitext(input_file)
        output_file = f"{base}_out{ext if ext else '.yaml'}"

    # Read the input YAML file.
    with open(input_file, "r") as file:
        data = yaml.safe_load(file)

    # Reorder the keys in the YAML data.
    new_data = reorder_keys(data)

    # Write the updated YAML data to the output file, preserving the key order.
    with open(output_file, "w") as file:
        yaml.dump(new_data, file, sort_keys=False)

    print(f"YAML file has been successfully processed.\nReordered keys: x, y, z, t, g.\nOutput saved to: {output_file}")


if __name__ == '__main__':
    main()
