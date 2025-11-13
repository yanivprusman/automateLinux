declare -A COLORS=(
  [red]="0;31"
  [green]="0;32"
  [yellow]="0;33"
  [blue]="0;34"
  [NC]="0"  
)
for color in "${!COLORS[@]}"; do
    declare "$color"="${COLORS[$color]}"
    declare "_$color"=$'\['"\e[${COLORS[$color]}m"$'\]'
    export "$color" "_$color"
done
