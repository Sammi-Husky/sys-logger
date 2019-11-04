make clean && make
ren %2 "exefs.nsp"
printf "\n passive \n cd atmosphere/titles/4200000000696969 \n put exefs.nsp \n ls \n quit \n" | FTP %1 5000