import os
import sys
import subprocess
from pathlib import Path
from shutil import copy2

import Utils

from io import BytesIO
from urllib.request import urlopen

class VulkanConfiguration:
    requiredVulkanVersion = "1.3.211.0"
    newVulkanInstallerVersion = "1.3.211.0"
    vulkanDirectory = "./Vulkan-Core/vendor/VulkanSDK"
    dependenciesFile = "Dependencies.lua"

    @classmethod
    def Validate(cls):
        if (not cls.CheckVulkanSDK()):
            print("Vulkan SDK not installed correctly.")
            return
            
        if (not cls.CheckVulkanSDKDebugLibs()):
            print("Vulkan SDK debug libs not found.")

    @classmethod
    def CheckVulkanSDK(cls):
        vulkanSDK = os.environ.get("VULKAN_SDK")
        if (vulkanSDK is None):
            print("\nYou don't have the Vulkan SDK installed!")
            cls.__InstallVulkanSDK()
            return False
        else:
            print(f"\nLocated Vulkan SDK at {vulkanSDK}")

        if (cls.requiredVulkanVersion not in vulkanSDK):
            print(f"You don't have the correct Vulkan SDK version! (Engine requires {cls.requiredVulkanVersion})")
            cls.__InstallVulkanSDK()
            return False
    
        print(f"Correct Vulkan SDK located at {vulkanSDK}")
        return True

    @classmethod
    def __InstallVulkanSDK(cls):
        permissionGranted = False
        while not permissionGranted:
            reply = str(input("Would you like to install VulkanSDK {0:s}? [Y/N]: ".format(cls.requiredVulkanVersion))).lower().strip()[:1]
            if reply == 'n':
                return
            permissionGranted = (reply == 'y')

        vulkanInstallURL = f"https://sdk.lunarg.com/sdk/download/{cls.requiredVulkanVersion}/windows/VulkanSDK-{cls.requiredVulkanVersion}-Installer.exe"
        vulkanPath = f"{cls.vulkanDirectory}/VulkanSDK-{cls.requiredVulkanVersion}-Installer.exe"
        print("Downloading {0:s} to {1:s}".format(vulkanInstallURL, vulkanPath))
        Utils.DownloadFile(vulkanInstallURL, vulkanPath)
        print("Running Vulkan SDK installer...")
        os.startfile(os.path.abspath(vulkanPath))
        print("Re-run this script after installation!")
        os.remove(os.path.abspath(vulkanPath))
        quit()

    @classmethod
    def CheckVulkanSDKDebugLibs(cls):
        if int(cls.requiredVulkanVersion.replace('.', '')) >= int(cls.newVulkanInstallerVersion.replace('.', '')):
            vulkanSDK = os.environ.get("VULKAN_SDK")
            subprocess.run([f"{vulkanSDK}/maintenancetool.exe", "--al", "-c", "in", "com.lunarg.vulkan.debug"])

            with open(cls.dependenciesFile, 'r', encoding='utf-8') as file:
                dependencies = file.readlines()
            
            with open(cls.dependenciesFile, 'w+', encoding='utf-8') as file:
                for line in dependencies:
                    if line.startswith("LibraryDir[\"VulkanSDK_Debug\"]"):
                        line = "LibraryDir[\"VulkanSDK_Debug\"] = \"%{VULKAN_SDK}/Lib\"\n"
                    if line.startswith("LibraryDir[\"VulkanSDK_DebugDLL\"]"):
                        line = "LibraryDir[\"VulkanSDK_DebugDLL\"] = \"%{VULKAN_SDK}/Bin\"\n"

                    file.write(line)

            return True
        else:
            shadercdLib = Path(f"{cls.vulkanDirectory}/Lib/shaderc_sharedd.lib")
            
            VulkanSDKDebugLibsURLlist = [
                f"https://sdk.lunarg.com/sdk/download/{cls.requiredVulkanVersion}/windows/VulkanSDK-{cls.requiredVulkanVersion}-DebugLibs.zip",
                f"https://files.lunarg.com/SDK-{cls.requiredVulkanVersion}/VulkanSDK-{cls.requiredVulkanVersion}-DebugLibs.zip"
            ]
            
            if not shadercdLib.exists():
                print(f"\nNo Vulkan SDK debug libs found. (Checked {shadercdLib})")
                vulkanPath = f"{cls.vulkanDirectory}/VulkanSDK-{cls.requiredVulkanVersion}-DebugLibs.zip"
                Utils.DownloadFile(VulkanSDKDebugLibsURLlist, vulkanPath)
                print("Extracting", vulkanPath)
                Utils.UnzipFile(vulkanPath, deleteZipFile=True)
                print(f"Vulkan SDK debug libs installed at {os.path.abspath(cls.vulkanDirectory)}")

                with open(cls.dependenciesFile, 'r', encoding='utf-8') as file:
                    dependencies = file.readlines()
            
                with open(cls.dependenciesFile, 'w+', encoding='utf-8') as file:
                    for line in dependencies:
                        if line.startswith("LibraryDir[\"VulkanSDK_Debug\"]"):
                            line = "LibraryDir[\"VulkanSDK_Debug\"] = \"%{wks.location}/Vulkan-Core/vendor/VulkanSDK/Lib\"\n"
                        if line.startswith("LibraryDir[\"VulkanSDK_DebugDLL\"]"):
                            line = "LibraryDir[\"VulkanSDK_DebugDLL\"] = \"%{wks.location}/Vulkan-Core/vendor/VulkanSDK/Bin\"\n"

                        file.write(line)
            else:
                print(f"\nVulkan SDK debug libs located at {os.path.abspath(cls.vulkanDirectory)}")
            return True

if __name__ == "__main__":
    VulkanConfiguration.Validate()
