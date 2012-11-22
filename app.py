from flask import Flask, render_template
from functools import wraps

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

@app.route("/qualifications")
def qualifications():
    return render_template('qualifications.jinja2', active_page='qualifications')

@app.route("/approach")
def approach():
    return render_template('approach.jinja2', active_page='approach')

@app.route("/fees")
def fees():
    return render_template('fees.jinja2', active_page='fees')

@app.route("/contact")
def contact():
    return render_template('contact.jinja2', active_page='contact')

if __name__ == "__main__":
    app.debug = True
    app.run('0.0.0.0', 5000)
