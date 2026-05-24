import os

class Config:
    DATABASE = os.path.join(os.path.dirname(__file__), 'spyware.db')
    DEBUG = False

class DevelopmentConfig(Config):
    DEBUG = True
