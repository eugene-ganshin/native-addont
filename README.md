NPM PACKAGE

To run the package install CPP compiler and python 3.\*

To run follow the instruction:

- cd into new folder
- npm init -y
- tsc --init
- touch index.ts
- npm i native-addon
- import \* as addon from 'native-addon'
- console.log(addon.greet(1,2))
- should log 2
