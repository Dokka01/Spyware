import os


class Config:
    # Chemin vers le fichier SQLite.
    # En Docker, on peut passer DATABASE_PATH en variable d'environnement pour pointer vers un volume.
    DATABASE = os.environ.get(
        'DATABASE_PATH',
        os.path.join(os.path.dirname(__file__), 'spyware.db')
    )
    DEBUG = False


class DevelopmentConfig(Config):
    DEBUG = True
