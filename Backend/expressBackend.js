const express = require('express');
const dotenv = require('dotenv');
dotenv.config({path: '.env_vars'});
const db = require('./modules/db');

const PORT = process.env.PORT || '3306';




const app = express();
const bodyParser = require("body-parser");

const result = db.pool.query("select * from testTable");
const aquariumRes = db.pool.query("select * from aquarium");

console.log(result);


app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "http://localhost:4200"); // update to match the domain you will make the request from
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

//BigInt fix needed for INSERT
BigInt.prototype.toJSON = function() {
    return this.toString();
  };

//ROUTES

app.get('/', (request, response) => {
    response.status(200).send("This is not why you're here. Head to /aquarium")
})

app.get('/aquarium', async function(req, res){
	try {
		const sqlQuery = 'SELECT id, temp, tds, ph FROM aquarium';
		const rows = await db.pool.query(sqlQuery);
		res.set('Access-Control-Allow-Origin', 'http://localhost:4200');
		res.status(200).json(rows);
	} catch (error) {
		res.status(400).send(error.message);
	}
	
})

app.get('/aqlast/:count', async function(req, res){
	try {
		const sqlQuery = 'SELECT id, temp, tds, ph, ts_insert FROM `aquarium` WHERE id > (SELECT MAX(id) - ? FROM `aquarium`)';
		const rows = await db.pool.query(sqlQuery, req.params.count);
		res.set('Access-Control-Allow-Origin', 'http://localhost:4200');
		res.status(200).json(rows);
	} catch (error) {
		res.status(400).send(error.message);
	}
	
})

// POST
app.post('/aquarium', async (req, res) => {
    let val = req.body;
    try {
        const result = await db.pool.query("INSERT INTO aquarium (id, temp, ph, tds, ts_insert) VALUES (0, ?, ?, ?, NULL)", [val.temp, val.ph, val.tds]);
        res.set('Access-Control-Allow-Origin', 'http://localhost:4200');
        res.send(result);
        //res.status(200);
    } catch (err) {
        throw err;
    }
});

app.get('/all', async function(req,res){
    try {
        const sqlQuery = 'SELECT id, text, timeStamp FROM testTable';
        const rows = await db.pool.query(sqlQuery, req.params.id);
        //res.status(200).json(rows);
        res.status(200).json({id:req.params.id})
    } catch (error) {
        res.status(400).send(error.message)
    }


    //res.status(200).json({id:req.params.id})
});


app.get('/lightTimes', async function(req, res){
	try {
		const sqlQuery = 'SELECT id, timeOn, timeOff, profileOn FROM lights WHERE id = (SELECT MAX(id) FROM lights)';
		const rows = await db.pool.query(sqlQuery);
		res.set('Access-Control-Allow-Origin', 'http://localhost:4200');
		res.status(200).json(rows);
	} catch (error) {
		res.status(400).send(error.message);
	}
	
})

app.post('/lightTimes', async (req, res) => {
    let val = req.body;
    try {
		console.log('vals:');
		console.log(val.timeOn);
		console.log(val.timeOff);
		console.log(val.profileOn);
		console.log('------');
        const result = await db.pool.query("INSERT INTO lights (id, timeOn, timeOff, profileOn) VALUES (0, ?, ?, ?)", [val.timeOn, val.timeOff, val.profileOn]);
        res.set('Access-Control-Allow-Origin', 'http://localhost:4200');
        res.send(result);
		console.log('POST:');
		
        //res.status(200);
    } catch (err) {
        throw err;
    }
});

app.get('/:id', async function(req,res){
    try {
        const sqlQuery = 'SELECT id, text, timeStamp FROM testTable WHERE id=?';
        const rows = await db.pool.query(sqlQuery, req.params.id);
        res.status(200).json(rows);
    } catch (error) {
        res.status(400).send(error.message)
    }


    //res.status(200).json({id:req.params.id})
});

//const userRouter = require('./testModules/user');
//app.use('/user',userRouter);



/**Start listening */
app.listen(PORT, () => {
    console.log(`Listening for requests on port ${PORT}`)
})
