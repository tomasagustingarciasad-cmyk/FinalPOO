    # Instalar npm
sudo apt install -y nodejs npm

# Para poner en marcha el servidor se deben instalar las dependencias y luego levantar el servicio
cd cliente-web/
npm install
npm run dev
<!-- 
npm install es el comando encargado de instalar las dependencias del proyecto.
npm run dev es el comando encargado de poner el servicio en marcha, con este comando levantamos el cliente web -->

# Comandos para levantar el docker y crear la base de datos:
docker run --name finalpoo -p 31432:5432 -e POSTGRES_PASSWORD=pass123 -d postgres
docker exec -i finalpoo createdb -U postgres poo

<!-- Esto lo que hace es crear un contenedor de docker que tiene nombre finalpoo, mapea el puerto interno del contendor (5432) al puesto externo pc (31432). Las conexiones se hacen al puerto 31432. La contraseña para la conexion es pass123 y el usuario postgres. Luego crea la base de datos dentro del contenedor con el usuario postgres y nombre de la base de datos poo.  -->

# Crear esquema, tabla de usuario e insertar usuarios
psql "postgresql://postgres:pass123@localhost:31432/poo" -v ON_ERROR_STOP=1 -f db/init.sql

<!-- Esto lo que hace es ejecutar dentro del contenedor y en la base de datos el archivo init.sql. Se pueden crear mas archivos .sql que ejecuten otros comandos y lo unico que debe ser cambiado es la referencia final al archivo que se quiere ejecutar.  -->

# Instalar libreria utilizada (c++) para hacer la conexion a la base de datos
sudo apt install -y libpqxx-dev g++

g++ -std=c++17 main_pqxx.cpp -lpqxx -lpq -o app_pqxx
./app_pqxx
