import os
import shutil

def clear_folders(base_dir):
    # Walk through the directory
    for root, dirs, files in os.walk(base_dir):
        for dir_name in dirs:
            # Check for 'out' and '.vs' folders
            if dir_name == 'out' or dir_name == '.vs':
                folder_path = os.path.join(root, dir_name)
                print(f"Removing folder: {folder_path}")
                shutil.rmtree(folder_path)  # Remove the folder and all its contents

if __name__ == "__main__":
    # base_directory = input("Enter the base directory to clear 'out/' and '.vs/' folders: ")
    base_directory = "./"
    clear_folders(base_directory)
    print("Clearing complete. All out/ and .vs/ folders have been removed.")
