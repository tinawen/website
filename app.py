from flask import Flask, render_template
from functools import wraps
import os

app = Flask(__name__)

def check_auth(username, password):
    """This function is called to check if a username /
    password combination is valid.
    """
    return username == 'recruiting' and password == 'recruitingisawesome'

def authenticate():
    """Sends a 401 response that enables basic auth"""
    return Response(
    'Could not verify your access level for that URL.\n'
    'You have to login with proper credentials', 401,
    {'WWW-Authenticate': 'Basic realm="Login Required"'})

def requires_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth = request.authorization
        if not auth or not check_auth(auth.username, auth.password):
            return authenticate()
        return f(*args, **kwargs)
    return decorated

@app.route("/")
@app.route("/home")
def home():
    return render_template('home.jinja2', active_page='home')

@app.route("/bio")
def bio():
    return render_template('bio.jinja2', active_page='bio')

@app.route("/resume")
def resume():
    return render_template('resume.jinja2', active_page='resume')

@app.route("/projects")
def projects():
    return render_template('projects.jinja2', active_page='projects')

@app.route("/menu")
def menu():
    return render_template('projects/menu.jinja2', active_page='projects')

@app.route("/connect4")
def connect4():
    return render_template('projects/connect4.jinja2', active_page='projects')

@app.route("/file_browser")
def file_browser():
    return render_template('projects/file_browser.jinja2', active_page='projects')

@app.route("/chrome_extension")
def chrome_extension():
    return render_template('projects/chrome_extension.jinja2', active_page='projects')

if __name__ == "__main__":
    port = os.getenv('PORT')
    if port is not None:
        app.run('0.0.0.0', int(port))
    else:
        app.debug = True
        app.run('0.0.0.0', 5000)
