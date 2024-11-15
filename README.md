# DevLink

## Introduction :
DevLink est une plateforme de mise en relation entre freelances et clients. Conçu comme un projet d'étude, DevLink permet aux utilisateurs de s'inscrire en tant que freelance ou client, de consulter des profils, et de gérer les informations de contact. Ce projet vise à présenter une solution de base pour une plateforme de freelances, développée avec une API en C, une base de données MySQL, et une interface web simple en HTML/CSS.

### Installation : 

**Prérequis**

*MySQL*: Pour la gestion de la base de données.
*Navigateur web*: pour tester l'interface front-end en HTML/CSS.

*Librairies nécessaires* :

*libmicrohttpd* : Pour la gestion des requêtes HTTP dans l'API.
*libmysqlclient* : Pour la connexion à la base de données MySQL.

**Étapes d'installation**
Cloner le dépôt :

```
git clone <URL_du_dépôt>
cd DevLink

``` 

Configurer la base de données :
Créez une base de données devlink dans MySQL.

Configurez les tables nécessaires :

users : Contient les informations des utilisateurs (username, email, password, role).
freelances et clients : Tables spécifiques aux rôles avec des champs comme l’âge et les compétences pour les freelances.

Compiler l'API :

```
gcc -o devlink_api devlink_api.c -lmicrohttpd -lmysqlclient

``` 

Lancer l'API :

``` 
./devlink_api

``` 


Le serveur HTTP de l'API est maintenant actif sur le port 8888.

Lancer l'interface front-end :

Ouvrez le fichier index.html dans un navigateur pour voir la page d’accueil.
Le front-end utilise l'API pour afficher dynamiquement les informations sur les freelances et les clients.

## Usage

### Fonctionnalités principales
Inscription des utilisateurs : Les utilisateurs peuvent s'inscrire en tant que freelance ou client. Les informations sont stockées dans la base de données MySQL.

Affichage des freelances : La liste des freelances est récupérée depuis la base de données et affichée sur la page dédiée.

Affichage des clients : La liste des clients est également récupérée et affichée de manière similaire.

Exemples de requêtes de l'API

``` 
POST /register

``` 

Enregistre un utilisateur en tant que freelance ou client.

``` 

POST /register
Content-Type: application/x-www-form-urlencoded

username=JohnDoe&email=johndoe%40example.com&password=secure123&role=freelance&age=30&skills=C%2C+Python
``` 

GET /freelancers

``` 
Récupère la liste des freelances enregistrés.

``` 
GET /freelancers

``` 
Récupère la liste des clients enregistrés.

``` 
GET /clients

``` 


Related Projects
Voici quelques projets similaires qui pourraient vous intéresser ou que vous pourriez utiliser comme référence :

Upwork : Plateforme de freelances pour l'inspiration en termes de fonctionnalités avancées.
Fiverr : Exemple de plateforme centrée sur des microservices et des interactions de type freelance-client.

Licensing
Ce projet est sous licence MIT, ce qui signifie que vous êtes libre de le modifier et de le redistribuer.