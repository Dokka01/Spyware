from flask import Flask
from flask_cors import CORS
from database import init_db
from routes.machines import machines_bp


def create_app():
    app = Flask(__name__)
    CORS(app)

    init_db()

    app.register_blueprint(machines_bp, url_prefix='/api/machines')

    return app
