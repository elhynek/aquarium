import { Injectable } from "@angular/core";
import { HttpClient } from "@angular/common/http";
import { environment } from "src/environment";

@Injectable({
    providedIn: 'root'
})
export class SharedServices{

    private dbUrl = environment.dburl;

    constructor(private httpClient: HttpClient){

    }

    public getAquariumValues(){
        
        return this.httpClient.get(this.dbUrl + 'aquarium');
    }

}