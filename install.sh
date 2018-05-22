#!/bin/bash
echo '- Welcome to the Pickle installation wizard'
echo
read -r -p "${1:-- You want to install g++ and make? [Yes/No]} " response
    case "$response" in
        [yY][eE][sS]|[yY])
        sudo apt-get install g++ make
    ;;
    esac
echo

echo "- Now, let's configure the execution file..."

sleep 2

make > /dev/null

pwd=$(pwd)
user=$(whoami)

cp $pwd/pickle /home/$user/.local/bin > /dev/null
export PATH=$PATH:/home/$user/.local/bin
source /etc/environment

echo
echo "- Thank's for install Pickle, run 'pickle' on terminal to start"
echo "
            ██████╗ ██╗ ██████╗██╗  ██╗██╗     ███████╗
            ██╔══██╗██║██╔════╝██║ ██╔╝██║     ██╔════╝
            ██████╔╝██║██║     █████╔╝ ██║     █████╗  
            ██╔═══╝ ██║██║     ██╔═██╗ ██║     ██╔══╝  
            ██║     ██║╚██████╗██║  ██╗███████╗███████╗
            ╚═╝     ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝" 
            echo "
             The Best Text Editor Text User Interface"
            echo
            echo