#ifndef FILE_ENUM_H
#define FILE_ENUM_H

/* Compresse le contenu d'un dossier cible dans un ZIP.
   folder_name : "Documents", "Images", "Videos", ou "Bureau" */
void enumerate_and_zip_folder(const char *folder_name, const char *zip_output_path);

#endif /* FILE_ENUM_H */
