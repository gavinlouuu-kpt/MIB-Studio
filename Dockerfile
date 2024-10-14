# Use the official Microsoft Visual Studio Build Tools image
FROM mcr.microsoft.com/windows/servercore:ltsc2019

# Set up environment to handle the installation of .NET Framework and other tools
SHELL ["cmd", "/S", "/C"]

# Create temp directory
RUN mkdir C:\temp

# Download and install .NET Framework 4.8
ADD https://download.visualstudio.microsoft.com/download/pr/2d6bb6b2-226a-4baa-bdec-798822606ff1/8494001c276a4b96804cde7829c04d7f/ndp48-x86-x64-allos-enu.exe C:/temp/ndp48-x86-x64-allos-enu.exe
RUN C:\temp\ndp48-x86-x64-allos-enu.exe /quiet /norestart && del C:\temp\ndp48-x86-x64-allos-enu.exe

# Install Chocolatey
RUN powershell -Command "Set-ExecutionPolicy Bypass -Scope Process -Force; \
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; \
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"

# Install Visual Studio Build Tools
RUN choco install visualstudio2019buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --includeOptional --quiet --norestart" -y

# Install CMake (optional, if your project uses CMake)
RUN choco install cmake -y

# Install other tools or dependencies as needed
# RUN choco install git -y

# Set the working directory
WORKDIR /app

# Copy your project files into the container (optional, if you want to include your project in the image)
# COPY . /app

# Set the entry point to a shell or a script that you want to run
CMD ["cmd.exe"]
