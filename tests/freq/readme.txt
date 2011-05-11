vbsim -d 36 0 0 0 -n 0 10 0 -o signal.ref
vbmakefilter -e exofilt.ref -t 36 1000 -lf 5 -hf 3
vbmm2 -applyfilter signal.ref exofilt.ref filtered1.ref
vbfilter -rl 5 -rh 3 signal.ref -p 1000 -o filtered2.ref
vecview filtered1.ref filtered2.ref signal.ref
rm signal.ref exofilt.ref filtered*.ref
