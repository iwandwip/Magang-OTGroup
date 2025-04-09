import os
import shutil
import argparse


def remove_pycache(root_dir, dry_run=False):
    """
    Recursively removes all '__pycache__' directories within the given root directory.

    This function walks through the directory tree starting at 'root_dir' and deletes
    every directory named '__pycache__'. If 'dry_run' is set to True, it will only print
    the directories that would be removed without actually deleting them.

    Parameters:
        root_dir (str): The path of the root directory to search for '__pycache__' folders.
        dry_run (bool): If True, no directories will be removed; only their paths will be printed.
    """
    for dirpath, dirnames, _ in os.walk(root_dir):
        # Create a copy of the list to avoid modifying while iterating.
        for dirname in list(dirnames):
            if dirname == '__pycache__':
                full_path = os.path.join(dirpath, dirname)
                print(f"Found __pycache__: {full_path}")
                if dry_run:
                    print(f"Dry run: would remove {full_path}")
                else:
                    try:
                        shutil.rmtree(full_path)
                        print(f"Removed: {full_path}")
                    except Exception as e:
                        print(f"Error removing {full_path}: {e}")
                    # Remove the directory from the list to prevent os.walk from processing it further.
                    dirnames.remove(dirname)


def main():
    """
    Main function to parse command-line arguments and execute the removal of '__pycache__' directories.

    Command-Line Arguments:
        --root      (Optional) The root directory to start searching for '__pycache__' folders.
                    Defaults to the current directory ('.') if not specified.
        --dry-run   (Optional) If specified, the program will only display which directories
                    would be removed, without actually deleting them.

    Example Usage:
        To remove all '__pycache__' folders in the current directory:
            python remove_pycache.py

        To remove all '__pycache__' folders in a specific directory:
            python remove_pycache.py --root /path/to/your/project

        To perform a dry run without deleting any directories:
            python remove_pycache.py --root /path/to/your/project --dry-run
    """
    parser = argparse.ArgumentParser(
        description='Recursively remove all __pycache__ folders from the specified directory.',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        '--root',
        required=False,
        default='.',
        help='Root directory to search for __pycache__ folders (default: current directory)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Display the directories that would be removed without deleting them.'
    )
    args = parser.parse_args()

    remove_pycache(args.root, args.dry_run)


if __name__ == '__main__':
    main()
