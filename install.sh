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

pwd=$(pwd)
user=$(whoami)

cd $pwd/usr/dev/
make -s > /dev/null

mv $pwd/usr/dev/pickle /$pwd/usr/bin

origin=$(pwd)
cd /../usr/bin/
target=$(pwd)
# sudo mv $origin/pickle $target

cp $target/pickle /home/$user/.local/bin > /dev/null

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