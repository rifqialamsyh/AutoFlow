import os
from flask import Flask
from handler import addData, showData, deleteData

app = Flask(__name__,static_folder='assets', template_folder='web')

#=============================================================================
# Data Functions
@app.route('/showData', methods=['GET'])
def showData_route():
    return showData()

@app.route('/addData', methods=['POST'])
def addData_route():
    return addData()

@app.route('/deleteData/<id>', methods=['DELETE'])
def deleteData_route(id):
    return deleteData(id)

if __name__ == '__main__':
    os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = "/key.json" 
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", 5000)))