use std::env;
use std::fs::metadata;
use std::net::{Ipv4Addr, SocketAddrV4};
use std::path::Path;

use crate::args;

use crate::http::*;
use crate::stats::*;

use clap::Parser;
use tokio::net::{TcpListener, TcpStream};
use tokio::io::AsyncReadExt;

use anyhow::Result;
use tokio::fs::File;


pub fn main() -> Result<()> {
    // Configure logging
    // You can print logs (to stderr) using
    // `log::info!`, `log::warn!`, `log::error!`, etc.
    env_logger::Builder::new()
        .filter_level(log::LevelFilter::Info)
        .init();

    // Parse command line arguments
    let args = args::Args::parse();

    // Set the current working directory
    env::set_current_dir(&args.files)?;

    // Print some info for debugging
    log::info!("HTTP server initializing ---------");
    log::info!("Port:\t\t{}", args.port);
    log::info!("Num threads:\t{}", args.num_threads);
    log::info!("Directory:\t\t{}", &args.files);
    log::info!("----------------------------------");
    println!("hello sohom");

    // Initialize a thread pool that starts running `listen`
    tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .worker_threads(args.num_threads)
        .build()?
        .block_on(listen(args.port))
}

async fn listen(port: u16) -> Result<()> {
    // Hint: you should call `handle_socket` in this function.
    println!("listen");

    let addr = format!("0.0.0.0:{}", port).parse::<SocketAddrV4>()?;
    let listener = TcpListener::bind(&addr).await?;

    while let Ok((socket, _)) = listener.accept().await {
        tokio::spawn(handle_socket(socket));
    }
    Ok(())
}

// Handles a single connection via `socket`.
async fn handle_socket(mut socket: TcpStream) -> Result<()> {
    println!("Handle socket");
    //parse request
    let parsed = parse_request(&mut socket).await?; 
    let file_path = format!(".{}", parsed.path); //append . to filepath
    let clone_path = file_path.clone();
    //convert to path
    let path = Path::new(&file_path);
    
    
    //println!("here");
    //let metadata = f.metadata().await?;
    //println!("metadata: {:?}", metadata);  
    //the path exists
    if path.exists() {
        //path is directory
        if path.is_dir() {
            println!("WE REACHED A DIRECTORY");

            //add a slash?
            let file_path2 = format!("{}/", file_path);
            //println!("filepath: {:?}", file_path2);  
            let complete_idx_path = format_index(&file_path2);
            let idx_path = Path::new(&complete_idx_path);
            //if the index.html file exists, output the contents
            if idx_path.exists() {
                println!("INDEX FILE EXISTS");

                let idx_path_clone = complete_idx_path.clone();
                let mut dir_idx = match File::open(complete_idx_path).await {
                    Ok(dir_idx) => dir_idx,
                    Err(_) => {
                        start_response(&mut socket, 404).await?;
                        end_headers(&mut socket).await?;
                        return Ok(()); 
                    }
                };
                let mime_type = get_mime_type(&idx_path_clone);
                //have to open the file
                //println!("mime type: {:?}", mime_type);
                let metadata = dir_idx.metadata().await?;
                let filesize = metadata.len().to_string(); //this is type u64, convert to string
                //println!("file size: {:?}", filesize);
                start_response(&mut socket, 200).await?;
                let h1 = "Content-Type";
                send_header(&mut socket, &h1, &mime_type).await?;
                let h2 = "Content-Length";
                send_header(&mut socket, &h2, &filesize).await?;
                end_headers(&mut socket).await?;      
                loop {
                    let mut buf = [0; 1024];
                    //println!("We made it to before bytes_read!");
                    let bytes_read = dir_idx.read(&mut buf).await?;
                    if bytes_read < 1024 {
                        socket.try_write(&buf[..bytes_read]);
                        break;
                    }
                    socket.try_write(&buf[..bytes_read]);
                }
            //index.html doesn't exist, so send hyperlinks to every file inside the directory
            } else {
                println!("INDEX FILE DOESNT EXIST");

                let cloned_clone_path = clone_path.clone();
                let mut entries = tokio::fs::read_dir(clone_path).await?;
                //send headers
                start_response(&mut socket, 200).await?;
                let mime_type = "text/html";
                let h1 = "Content-Type";
                send_header(&mut socket, &h1, &mime_type).await?;
                // let h2 = "Content-Length";
                // send_header(&mut socket, &h2, &filesize).await?;
                end_headers(&mut socket).await?;   
                //append . and .. to the list of hyperlinks first 
                let curr_dir = format_href(&cloned_clone_path, ".");
                //get parent by calling parent() on pathbuf object
                let parent = path.parent().unwrap();
                let parent_path_str = parent.as_os_str().to_str().unwrap();
                let parent_dir = format_href(&parent_path_str, "..");
                socket.try_write(&curr_dir.as_bytes());
                socket.try_write(&parent_dir.as_bytes());

                //let mut buf = [0; 1024];
                while let Some(entry) = entries.next_entry().await? {
                    //convert pathbuf to string
                    let path_buf = entry.path();
                    let path_buf_str = path_buf.into_os_string().into_string().unwrap();
                    // println!("pbs : {:?}", path_buf_str);  
                    // println!("fp : {:?}", fp);  
                    let filename = entry.file_name();
                    let filename_str = filename.to_str().unwrap();

                    //let borrow = format!("{}/", fp);
                    let formatted = format_href(&path_buf_str,&filename_str);
                    // let h1 = "Content-Type";
                    socket.try_write(&formatted.as_bytes());
                    // send_header(&mut socket, &h1, &mime_type).await?;
                }
            }
        //path is a file
        } else if path.is_file() {
            //open the file
            println!("WE REACHED A FILE");
            let clone_path = file_path.clone();
            let mut f = match File::open(file_path).await {
                Ok(f) => f,
                Err(_) => {
                    start_response(&mut socket, 404).await?;
                    end_headers(&mut socket).await?;
                    return Ok(()); 
                }
            };
            let mime_type = get_mime_type(&clone_path);
            //have to open the file
            //println!("mime type: {:?}", mime_type);
            let metadata = f.metadata().await?;
            let filesize = metadata.len().to_string(); //this is type u64, convert to string
            //println!("file size: {:?}", filesize);
            start_response(&mut socket, 200).await?;
            let h1 = "Content-Type";
            send_header(&mut socket, &h1, &mime_type).await?;
            let h2 = "Content-Length";
            send_header(&mut socket, &h2, &filesize).await?;
            end_headers(&mut socket).await?;      
            loop {
                let mut buf = [0; 1024];
                //println!("We made it to before bytes_read!");
                let bytes_read = f.read(&mut buf).await?;
                if bytes_read < 1024 {
                    socket.try_write(&buf[..bytes_read]);
                    break;
                }
                socket.try_write(&buf[..bytes_read]);
            }
        //path doesn't exist or another error occurred
        } else {
            println!("WE REACHED A ERROR");

            //serve 404
            start_response(&mut socket, 404).await?;
            end_headers(&mut socket).await?;
            return Ok(());
        }
    } else {
        println!("WE REACHED A ERROR");

            //serve 404
            start_response(&mut socket, 404).await?;
            end_headers(&mut socket).await?;
            return Ok(());
    } 
    Ok(())
}


//if path.exists()

// You are free (and encouraged) to add other funtions to this file.
// You can also create your own modules as you see fit.