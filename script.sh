#!/bin/bash
# wrap_namespace_correct.sh

NAMESPACE="OpenGLSomethingFrameDisplayerEVO"

find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
  -not -path "./libs/*" \
  -not -path "./imgui/*" \
  -not -path "./build/*" \
  -not -path "./.git/*" \
  -not -name "wrap_namespace_correct.sh" | while read file; do
  
  echo "Обработка: $file"
  
  # Временный файл
  temp_file="${file}.tmp"
  
  # Счетчики и флаги
  in_preprocessor_block=0
  wrote_namespace=0
  
  # Читаем исходный файл
  > "$temp_file"
  line_num=0
  
  while IFS= read -r line || [[ -n "$line" ]]; do
    line_num=$((line_num + 1))
    
    # Определяем, является ли строка препроцессорной директивой
    if [[ "$line" =~ ^[[:space:]]*#[[:space:]]* ]]; then
      in_preprocessor_block=1
      # Записываем как есть
      echo "$line" >> "$temp_file"
      continue
    fi
    
    # Если мы были в блоке препроцессора и наткнулись на не-препроцессорную строку
    if [[ $in_preprocessor_block -eq 1 ]] && [[ ! "$line" =~ ^[[:space:]]*$ ]]; then
      # Это первый не-препроцессорный, не-пустой код
      if [[ $wrote_namespace -eq 0 ]]; then
        echo "" >> "$temp_file"
        echo "namespace $NAMESPACE {" >> "$temp_file"
        echo "" >> "$temp_file"
        wrote_namespace=1
      fi
      in_preprocessor_block=0
    fi
    
    # Если строка пустая и мы еще в препроцессорном блоке
    if [[ "$line" =~ ^[[:space:]]*$ ]] && [[ $in_preprocessor_block -eq 1 ]]; then
      echo "$line" >> "$temp_file"
      continue
    fi
    
    # Записываем обычный код
    if [[ $wrote_namespace -eq 1 ]]; then
      echo "$line" >> "$temp_file"
    else
      # Если до сих пор не начали namespace (нет препроцессорных директив)
      if [[ ! "$line" =~ ^[[:space:]]*$ ]]; then
        echo "" >> "$temp_file"
        echo "namespace $NAMESPACE {" >> "$temp_file"
        echo "" >> "$temp_file"
        wrote_namespace=1
      fi
      echo "$line" >> "$temp_file"
    fi
    
  done < "$file"
  
  # Закрываем namespace если открывали
  if [[ $wrote_namespace -eq 1 ]]; then
    echo "" >> "$temp_file"
    echo "} // namespace $NAMESPACE" >> "$temp_file"
  fi
  
  # Заменяем исходный файл
  mv "$temp_file" "$file"
  
done

echo "Готово!"