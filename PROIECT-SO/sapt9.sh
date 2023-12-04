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
        if [[ $line =~ ^[A-Za-z0-9,[:space:]\.\!\?]+[^\,]$ && ! $line =~ ,[[:space:]]+și ]]; then
            # Incrementare contor
            ((counter++))
        fi
    fi
done

# Afiseaza rezultatul final
echo "Numarul de propozitii ce il contine pe $1 este: $counter"



