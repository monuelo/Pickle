confirm() {
    
    read -r -p "${1:--Are you sure you want to uninstall Pickle? [Yes/No]} " response
    case "$response" in
        [yY][eE][sS]|[yY])
        {
            pwd=$(pwd)
            user=$(whoami)
            rm /home/$user/.local/bin/pickle > /dev/null
        } || {
            echo '\n\033[0;31m(FAIL)\033[0m Uninstall failed'
        }
        show
                ;;
        *)
            show
            ;;
    esac
}

show() {
            echo
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
}

confirm