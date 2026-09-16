#!/bin/bash

# Mendefinisikan variabel direktori
ASSETS_DIR="assets"
DEST_DIR="data/assets/gz"

# Membuat direktori tujuan jika belum ada
mkdir -p "$DEST_DIR"

# Mengambil daftar file di dalam folder assets
files=()
for entry in "$ASSETS_DIR"/*; do
  if [ -f "$entry" ]; then
    files+=("$(basename "$entry")")
  fi
done

if [ ${#files[@]} -eq 0 ]; then
    echo "Folder $ASSETS_DIR kosong atau tidak ditemukan."
    exit 1
fi

# Array untuk menyimpan status pilihan (0 = tidak dipilih, 1 = dipilih)
selections=()
for i in "${!files[@]}"; do
    selections[$i]=0
done

cursor=0

render_menu() {
    clear
    echo "Pilih file yang akan dikompres"
    echo "(Panah Atas/Bawah: Navigasi | SPASI: Pilih/Batal | ENTER: Eksekusi)"
    echo "------------------------------------------------------------------"
    for i in "${!files[@]}"; do
        if [ "${selections[$i]}" -eq 1 ]; then
            checkbox="[*]"
        else
            checkbox="[ ]"
        fi

        if [ $i -eq $cursor ]; then
            echo -e " > \e[32m$checkbox ${files[$i]}\e[0m"
        else
            echo "   $checkbox ${files[$i]}"
        fi
    done
    echo "------------------------------------------------------------------"
}

while true; do
    render_menu
    
    # IFS= mencegah Bash mengabaikan karakter Spasi
    IFS= read -rsn1 key
    
    if [[ $key == $'\x1b' ]]; then
        read -rsn2 key2
        case "$key2" in
            '[A') # Panah Atas
                ((cursor--))
                if [ $cursor -lt 0 ]; then cursor=$((${#files[@]} - 1)); fi
                ;;
            '[B') # Panah Bawah
                ((cursor++))
                if [ $cursor -ge ${#files[@]} ]; then cursor=0; fi
                ;;
        esac
    elif [[ "$key" == " " ]]; then
        # Toggle pilihan dengan tombol Spasi
        if [ "${selections[$cursor]}" -eq 0 ]; then
            selections[$cursor]=1
        else
            selections[$cursor]=0
        fi
    elif [[ "$key" == "" ]]; then 
        # Tombol Enter
        break
    fi
done

clear

has_selection=0
for s in "${selections[@]}"; do
    if [ "$s" -eq 1 ]; then
        has_selection=1
        break
    fi
done

if [ "$has_selection" -eq 0 ]; then
    selections[$cursor]=1
fi

echo "Memproses file..."
echo "--------------------------------"

for i in "${!files[@]}"; do
    if [ "${selections[$i]}" -eq 1 ]; then
        filename="${files[$i]}"
        filepath="$ASSETS_DIR/$filename"
        destpath="$DEST_DIR/$filename.gz"
        
        echo "-> Mengompresi '$filename'..."
        
        # Mengambil ukuran file original dalam satuan byte
        size_before=$(stat -c%s "$filepath")
        
        gzip -k "$filepath"
        
        if [ $? -eq 0 ]; then
            mv "$filepath.gz" "$DEST_DIR/"
            
            # Mengambil ukuran file hasil kompresi dalam satuan byte
            size_after=$(stat -c%s "$destpath")
            
            # Menghitung persentase penghematan memori
            if [ "$size_before" -gt 0 ]; then
                saved=$(( 100 - (size_after * 100 / size_before) ))
            else
                saved=0
            fi
            
            echo "   Sukses dipindahkan ke $DEST_DIR/"
            echo -e "   Ukuran: \e[33m$size_before bytes\e[0m -> \e[32m$size_after bytes\e[0m (Hemat: $saved%)"
        else
            echo "   Error: Gagal mengompresi '$filename'"
        fi
        echo "" # Jarak antar file
    fi
done

echo "--------------------------------"
echo "Semua proses selesai!"