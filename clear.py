import os
import shutil
import sys
from colorama import init, Fore, Back

# Initialize colorama
init(autoreset=True)

def clear_folders(base_dir):
    # Walk through the directory
    has_error = False
    executed = False
    for root, dirs, files in os.walk(base_dir):
        for dir_name in dirs:
            # Check for 'out' and '.vs' folders
            if dir_name == 'out' or dir_name == '.vs' or dir_name == '.vscode':
                folder_path = os.path.join(root, dir_name)
                executed = True
                try:
                    shutil.rmtree(folder_path)  # Remove the folder and all its contents
                    print(f"{Fore.GREEN}[Success] Removing folder: {folder_path}")
                except PermissionError as e:
                    has_error = True
                    print(f"{Fore.RED}[Error]   Removing folder: {folder_path} ------ ", end="")
                    print(f"{Fore.RED}Details: {e}")

    return (not has_error, executed)

if __name__ == "__main__":
    # base_directory = input("Enter the base directory to clear 'out/' '.vscode' and '.vs/' folders: ")
    base_directory = "./"
    success, executed = clear_folders(base_directory)
    if success and executed:
        print("--------------------------------------------------")
        print(f"{Fore.GREEN}Clearing complete. All out/ .vscode/ and .vs/ folders have been removed.")
    elif success and not executed:
        print(f"{Fore.YELLOW}No need to clear. No out/ .vscode/ or .vs/ folders found.")
    else:
        if executed:
            print("--------------------------------------------------")
        print(f"{Fore.YELLOW}Clearing not complete. Some folders could not be removed.")
