#!/bin/bash
read=0
write=0
lock=0
unlock=0 
open=0
close=0
writebyte=0
readbyte=0
maxfile=0
maxbyte=0
maxcon=0
algrim=0
if [ -d "log" ]; then
   cd log
   file=$(ls)
   read=$(grep -o "RD" "$file" | wc -l)
   write=$(grep -o "WR" "$file" | wc -l)
   close=$(grep -o "CL" "$file" | wc -l)
   lock=$(grep -o "LCK" "$file" | wc -l)
   unlock=$(grep -o "ULK" "$file" | wc -l)
   algrim=$(grep -o "AR" "$file" | wc -l)
   maxfile=$(grep NF "$file" | cut -c 6-| sort -n | tail -1)
   maxbyte=$(grep NB "$file" | cut -c 6-| sort -n | tail -1)
   maxcon=$(grep  NC "$file" | cut -c 6-| sort -n | tail -1)
   
  
   for k in $(grep "WR" "$file" | cut -c 5-); do
      writebyte=$writebyte+$k
   done  
   for k in $(grep "RD" "$file" | cut -c 5-); do
      readbyte=$readbyte+$k
   done
  
   writebyte=$(bc <<< $writebyte)
   readbyte=$(bc <<< $readbyte)
   
   echo "numero di READ:" $read
   echo "numero di WRITE: "$write
   echo "numero di CLOSE: "$close
   echo "numero di LOCK: "$lock
   echo "numero di UNLOCK: "$unlock
   
   echo -e "-------------------------------------------------\\n"
   echo "numero di volte che è stato richiamato l'algoritmo di rimpiazamento" $algrim
   echo "dimensione massima in byte "$maxbyte
   echo "dimensione massima in numero di file "$maxfile
   echo "massimo numero di connessioni contemporaneamente "$maxcon
   echo  -e "-------------------------------------------------\\n" 
   mediawrbyte=0;
   mediardbyte=0;
   if [ $write -ne 0 ]; then
         mediawrbyte=$writebyte/$write
         mediawrbyte=$(bc <<< $mediawrbyte)
         echo "size media delle scritture" $mediawrbyte
   else
         echo "size media delle scritture 0"
   fi
   if [ $read -ne 0 ]; then
         mediardbyte=$readbyte/$read
         mediardbyte=$(bc <<< $mediardbyte)
         echo "size media delle letture " $mediardbyte
   else
         echo "size media delle letture 0"
   fi
   echo  -e "-------------------------------------------------\\n" 
   k=$(grep "TH" "$file" | cut -c 6- | sort | tail -1)    
   for ((i=1; i<=$k; i++)); do
      requestserv=$(grep "TH" "$file" | cut -c 6- | grep $i | wc -l)
      echo "worker thread $i richieste servite "$requestserv
   done
   echo  -e "-------------------------------------------------\\n" 
   echo -e "FILE RIMASTI NEL SERVER \\n"
  for k in $(grep "LF" "$file" | cut -c 4-); do 
      echo $k
  done
   
  
fi