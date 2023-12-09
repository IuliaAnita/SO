#!/bin/bash

# Verifica daca script-ul a primit exact un argument
if [ "$#" -ne 1 ]; then
    echo "Utilizare: $0 <caracter>"
    exit 1
fi

# Caracterul primit ca argument
character="$1"

# Initializare contor
counter=0

# Citeste linii de la intrarea standard pana la EOF
while IFS= read -r line || [ -n "$line" ]; do
    # Verifica daca linia incepe cu o litera mare
    if [[ $line =~ ^[A-Z] ]]; then
        # Verifica daca linia respecta conditiile date
        if [[ $line =~ ^[A-Z] ]] &&                 
	   [[ $line =~ [a-zA-Z0-9\ \!\?\.]+$ ]] &&       
	   [[ $line =~ (\.|\!|\?)$ ]] &&                  
	   [[ ! $line =~ ,\ și ]] &&
	   [[ $line == *"$character"* ]] ;
	then
            # Incrementare contor
            ((counter++))
        fi
    fi
done

# Afiseaza rezultatul final
echo "Numarul de propozitii corecte ce il contine pe $1 este: $counter"



