const express = require('express');
const dotenv = require('dotenv');
dotenv.config({path: '.env_vars'});
const db = require('./testModules/db');

const PORT = process.env.PORT || '3306';




const app = express();
const bodyParser = require("body-parser");
console.log("Test runnin");
console.log(process.env.DB_HOST);

const result = db.pool.query("select * from testTable");
const aquariumRes = db.pool.query("select * from aquarium");

console.log(result);


app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));


 /**
 * Middleware
 */
//app.use(express.json());
//app.use(express.urlencoded({extended:false}));

/**
 * Routes
 */

app.get('/', (request, response) => {
    response.status(200).send("This is not why you're here. Head to /user/:id and replace :id with your user id")
})

app.get('/aquarium', async function(req, res){
	try {
		const sqlQuery = 'SELECT id, temp, tds, ph FROM aquarium';
		const rows = await db.pool.query(sqlQuery);
		res.status(200).json(rows);
	} catch (error) {
		res.status(400).send(error.message);
	}
	
})

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
