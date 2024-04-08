from flask import Flask, request, jsonify
from flaskext.mysql import MySQL
from datetime import datetime
from nanoid import generate

app = Flask(__name__)

mysql = MySQL()
# Konfigurasi koneksi MySQL
app.config['MYSQL_DATABASE_USER'] = 'yourUSER'
app.config['MYSQL_DATABASE_PASSWORD'] = 'yourPASSWORD'
app.config['MYSQL_DATABASE_DB'] = 'yourDATABASE'
app.config['MYSQL_DATABASE_HOST'] = 'yourHOST'

mysql.init_app(app)

def showData():
    # Koneksi MySQL
    conn = mysql.connect()
    cursor = conn.cursor()

    query = "SELECT * FROM data"
    
    try:
        cursor.execute(query)
        rows = cursor.fetchall()
        results = []
        for row in rows:
            result = {
                'id': row[0],
                'temperature': row[1],
                'soil_moisture': row[2],
                'amount_of_water': row[3],
                'date': row[4],
            }
            results.append(result)

        return jsonify(results)

    except Exception as e:
        conn.close()
        response = {
            'status': 'error',
            'message': 'Terjadi kesalahan saat mengambil data',
            'error': str(e)
        }
        return jsonify(response)

def addData():
    data = request.get_json()

    id = generate(size=7)
    temperature = data['temperature']
    soil_moisture = data['soil_moisture']
    amount_of_water = data['amount_of_water']
    date = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # Koneksi MySQL
    conn = mysql.connect()
    cursor = conn.cursor()

    try:
        cursor.execute("INSERT INTO data (id, temperature, soil_moisture, amount_of_water, date) VALUES (%s, %s, %s, %s, %s)", 
                       (id, temperature, soil_moisture, amount_of_water, date))

        conn.commit()
        conn.close()

        response = {
            'status': 'success',
            'message': 'Data berhasil ditambahkan'
        }

        return jsonify(response)
    except Exception as e:
        conn.rollback()
        conn.close()
        response = {
            'status': 'error',
            'message': 'Terjadi kesalahan saat menambahkan data',
            'error': str(e)
        }
        return jsonify(response)

def deleteData(id):
    # Koneksi MySQL
    conn = mysql.connect()
    cursor = conn.cursor()

    try:
        cursor.execute("DELETE FROM data WHERE id = %s", (id,))

        conn.commit()
        conn.close()

        response = {
            'status': 'success',
            'message': 'Data berhasil dihapus'
        }

        return jsonify(response)
    except Exception as e:
        conn.rollback()
        conn.close()
        response = {
            'status': 'error',
            'message': 'Terjadi kesalahan saat menghapus data',
            'error': str(e)
        }
        return jsonify(response)
