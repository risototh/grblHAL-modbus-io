# fork https://github.com/grblHAL/core as https://github.com/risototh/grblHAL-core
# fork https://github.com/grblHAL/iMXRT1062 as https://github.com/risototh/iMXRT1062
# create https://github.com/risototh/grblHAL-modbus-io

# clone
git clone --recurse-submodules https://github.com/risototh/iMXRT1062.git grblHAL
cd grblHAL/

# add plugin as submodule and add .DS_Store between ignored
git submodule add https://github.com/risototh/grblHAL-modbus-io grblHAL_Teensy4/src/mbio
echo ".DS_Store" >> .gitignore 
git add .gitignore
git commit -m "adding MBIO module, ignore .DS_Store"
git push origin master

# alter the core submodule to point to risototh/grblHAL-core
git config --file=.gitmodules submodule.grblHAL_Teensy4/src/grbl.url https://github.com/risototh/grblHAL-core
git config --file=.gitmodules submodule.grblHAL_Teensy4/src/grbl.branch master
git submodule sync
git submodule update --init --recursive --remote
git add .gitmodules
git commit -m "altered core submodule to point to risototh/grblHAL-core"
git push origin master



git pull --recurse-submodules